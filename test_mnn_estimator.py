import math
import librosa
import soundfile
import torch
import torch.nn.functional as F
import numpy as np
import MNN
import time


def pad_and_partition(tensor, T):
    """
    pads zero and partition tensor into segments of length T

    Args:
        tensor(Tensor): BxCxFxL

    Returns:
        tensor of size (B*[L/T] x C x F x T)
    """
    old_size = tensor.size(3)
    new_size = math.ceil(old_size/T) * T
    tensor = F.pad(tensor, [0, new_size - old_size])
    [b, c, t, f] = tensor.shape
    split = new_size // T
    return torch.cat(torch.split(tensor, T, dim=3), dim=0)


class MNN_Estimator():
    def __init__(self,):
        # stft config
        self.F = 1024
        self.T = 512
        self.win_length = 4096
        self.hop_length = 1024
        self.win = torch.nn.Parameter(
            torch.hann_window(self.win_length),
            requires_grad=False
        )

        # filter
        self.sessions = []
        self.interpreters = []

        interpreter = MNN.Interpreter('./MNN/vocal.mnn')
        self.interpreters.append(interpreter)
        session = interpreter.createSession()
        self.sessions.append(session)

        interpreter = MNN.Interpreter('./MNN/accompaniment.mnn')
        self.interpreters.append(interpreter)
        session = interpreter.createSession()
        self.sessions.append(session)

    def compute_stft(self, wav):
        """
        Computes stft feature from wav

        Args:
            wav (Tensor): B x L
        """
        stft = torch.stft(
            wav, self.win_length, hop_length=self.hop_length, window=self.win, center=True, return_complex=False, pad_mode='constant')

        # only keep freqs smaller than self.F
        stft = stft[:, :self.F, :, :]
        real = stft[:, :, :, 0]
        im = stft[:, :, :, 1]
        mag = torch.sqrt(real ** 2 + im ** 2)
        return stft, mag


    def inverse_stft(self, stft):
        """Inverses stft to wave form"""
        stft_complex = torch.view_as_complex(stft)
        pad = self.win_length // 2 + 1 - stft_complex.size(1)
        stft_complex = F.pad(stft_complex, (0, 0, 0, pad))

        wav = torch.istft(stft_complex, self.win_length, hop_length=self.hop_length, center=True, window=self.win)

        return wav.detach()
    

    def separate(self, wav):
        """
        Separates stereo wav into different tracks corresponding to different instruments

        Args:
            wav (tensor): 2 x L
        """

        # stft - 2 X F x L x 2
        # stft_mag - 2 X F x L
        stft, stft_mag = self.compute_stft(wav)

        L = stft.size(2)

        # 1 x 2 x F x T
        stft_mag = stft_mag.unsqueeze(-1).permute([3, 0, 1, 2])
        stft_mag = pad_and_partition(stft_mag, self.T)  # B x 2 x F x T
        stft_mag = stft_mag.transpose(2, 3)  # B x 2 x T x F

        B = stft_mag.shape[0]

        # compute instruments' mask
        masks = []
        for interpreter, session in zip(self.interpreters, self.sessions):
            input_tensor = interpreter.getSessionInput(session)
            output_tensor = interpreter.getSessionOutput(session)
            # Create a list to save the output data for all batches
            
            batch_data = []

            # Process each batch
            for i in range(B):
                # Get the data for the current batch
                input_data = stft_mag.numpy()[i:i+1]  # Keep the batch dimension as 1

                # Convert the NumPy array to an MNN Tensor
                tmp_input = MNN.Tensor((1, 2, 512, 1024), MNN.Halide_Type_Float, \
                                    input_data, MNN.Tensor_DimensionType_Caffe)

                # Copy the data into the MNN Tensor
                input_tensor.copyFrom(tmp_input)

                # Perform inference
                interpreter.runSession(session)

                # Get the output data
                tmp_output = MNN.Tensor((1, 2, 512, 1024), MNN.Halide_Type_Float, \
                                        np.zeros((1, 2, 512, 1024)).astype(np.float32), MNN.Tensor_DimensionType_Caffe)
                output_tensor.copyToHostTensor(tmp_output)

                # Convert the output data for the current batch to a torch.Tensor
                output_data = torch.tensor(tmp_output.getNumpyData(), dtype=torch.float32)
                # Add the output Tensor for the current batch to the batches list
                batch_data.append(output_data)

            mask = torch.stack(batch_data).squeeze(dim=1)
            
            masks.append(mask)

        # compute denominator
        mask_sum = sum([m ** 2 for m in masks])
        mask_sum += 1e-10

        wavs = []
        for mask in masks:
            mask = (mask ** 2 + 1e-10/2)/(mask_sum)
            mask = mask.transpose(2, 3)  # B x 2 X F x T

            mask = torch.cat(
                torch.split(mask, 1, dim=0), dim=3)

            mask = mask.squeeze(0)[:,:,:L].unsqueeze(-1) # 2 x F x L x 1
            stft_masked = stft *  mask
            wav_masked = self.inverse_stft(stft_masked)

            wavs.append(wav_masked)

        return wavs
    

if __name__ == '__main__':
    sr = 44100
    es = MNN_Estimator()

    wav, _ = librosa.load('./coc.wav', mono=False, res_type='kaiser_fast', sr=sr)
    wav = torch.Tensor(wav)

    audio_duration = wav.shape[1] / sr

    start_time = time.time()
    wavs = es.separate(wav)
    end_time = time.time()
    inference_time = round(end_time - start_time, 6)
    real_time_factor = round(inference_time / audio_duration, 6)

    print(f"Inference time: {inference_time} seconds")
    print(f"Real-time factor: {real_time_factor} seconds")

    for i in range(len(wavs)):
        fname = 'output/out_{}.wav'.format(i)
        print("\033[32m" + f'{fname}' + "\033[0m")
        audio_data = wavs[i].detach().numpy().T
        max_val = np.max(np.abs(audio_data))
        if max_val > 0:
            audio_data = audio_data / max_val
        soundfile.write(fname, audio_data, sr, "PCM_16")

