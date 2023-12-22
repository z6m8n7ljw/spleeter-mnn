import math
import librosa
import soundfile
import numpy as np
import MNN
import time


def pad_and_partition(array, T):
    """
    pads zero and partition tensor into segments of length T

    Args:
        array(ndarray): BxCxFxL

    Returns:
        array of size (B*[L/T] x C x F x T)
    """
    old_size = array.shape[3]
    new_size = math.ceil(old_size / T) * T
    
    pad_width = [(0, 0)] * len(array.shape)
    pad_width[3] = (0, new_size - old_size)
    array = np.pad(array, pad_width, mode='constant', constant_values=0)
  
    split = new_size // T

    return np.concatenate(np.split(array, split, axis=3), axis=0)


def istft(stft_complex, win_length, hop_length, window):
    ifft_result = np.fft.irfft(stft_complex, n=win_length, axis=1)
    ifft_result = ifft_result * window[:, np.newaxis]

    time_array = np.zeros((ifft_result.shape[0], win_length + (ifft_result.shape[2] - 1) * hop_length))
    for i in range(ifft_result.shape[2]):
        start = i * hop_length
        end = start + win_length
        time_array[:, start:end] += ifft_result[:, :, i]

    return time_array


class MNN_Estimator():
    def __init__(self,):
        # stft config
        self.F = 1024
        self.T = 512
        self.win_length = 4096
        self.hop_length = 1024
        self.win = np.hanning(self.win_length)

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
            wav(ndarray): C x L
        """
        stft_channels = []
        mag_channels = []
        for i in range(wav.shape[0]):  # Iterate over channels
            stft_mono = librosa.stft(wav[i], n_fft=self.win_length, hop_length=self.hop_length, window=self.win, center=True)
            # only keep freqs smaller than self.F
            stft_real = stft_mono.real[:self.F, :]  
            stft_imag = stft_mono.imag[:self.F, :]
            stft = np.stack((stft_real, stft_imag), axis=-1) 
            mag = np.abs(stft_mono)[:self.F, :]
            stft_channels.append(stft)
            mag_channels.append(mag)
        # Convert the list of numpy arrays to a single numpy array with the correct shape
        stft_stereo = np.stack(stft_channels, axis=0) 
        mag_stereo = np.stack(mag_channels, axis=0)
        return stft_stereo, mag_stereo


    def compute_istft(self, stft):
        """Inverses stft to wave form"""
        # stft: B x F x (1 + L//hop_length) x 2
        # stft_complex: B x F x (1 + L//hop_length)
        real = stft[..., 0]
        im = stft[..., 1]
        stft_complex = real + 1j * im

        pad = self.win_length // 2 + 1 - stft_complex.shape[1]
        # stft_complex: B x (win_length//2 + 1) x (1 + L//hop_length)
        stft_complex = np.pad(stft_complex, pad_width=[(0, 0), (0, pad), (0, 0)], mode='constant', constant_values=(0, 0))
        # wav: B x L
        wav = istft(stft_complex, self.win_length, self.hop_length, self.win)

        return wav
    

    def separate(self, wav):
        """
        Separates stereo wav into different tracks corresponding to different instruments

        Args:
            wav(ndarray): C x L (C: Channels, L: Number of samples)
        """

        # stft: C X F x (1 + L//hop_length) x 2 (F: Number of frequency bins)
        # stft_mag: C X F x (1 + L//hop_length) (F: Number of frequency bins)
        stft, stft_mag = self.compute_stft(wav)

        L = stft.shape[2]

        stft_mag = np.expand_dims(stft_mag, axis=-1)
        stft_mag = np.transpose(stft_mag, axes=(3, 0, 1, 2)) # 1 x C x F x (1 + L//hop_length)
        stft_mag = pad_and_partition(stft_mag, self.T)  # B x C x F x T (B: Batch, T: Number of time frames)
        stft_mag = np.transpose(stft_mag, axes=(0, 1, 3, 2))  # B x C x T x F

        B = stft_mag.shape[0]

        # compute instruments' mask
        masks = []
        for interpreter, session in zip(self.interpreters, self.sessions):
            input_tensor = interpreter.getSessionInput(session)
            interpreter.resizeTensor(input_tensor, (B, 2, 512, 1024))
            interpreter.resizeSession(session)
            output_tensor = interpreter.getSessionOutput(session)

            tmp_input = MNN.Tensor([B, 2, 512, 1024], MNN.Halide_Type_Float, \
                                stft_mag, MNN.Tensor_DimensionType_Caffe)

            input_tensor.copyFrom(tmp_input)

            interpreter.runSession(session)

            tmp_output = MNN.Tensor([B, 2, 512, 1024], MNN.Halide_Type_Float, \
                                    np.zeros([B, 2, 512, 1024]).astype(np.float32), MNN.Tensor_DimensionType_Caffe)
            output_tensor.copyToHostTensor(tmp_output)
            mask = np.copy(tmp_output.getNumpyData()) # B x C x T x F
            masks.append(mask)

        # compute denominator
        mask_sum = np.sum([mask ** 2 for mask in masks], axis=0)
        mask_sum += 1e-10

        wavs = []
        for mask in masks:
            mask = (mask ** 2 + 1e-10 / 2) / mask_sum
            mask = np.transpose(mask, (0, 1, 3, 2))  # B x 2 X F x T

            splits = np.split(mask, indices_or_sections=mask.shape[0], axis=0)
            mask = np.concatenate(splits, axis=3)
            
            mask = mask.squeeze(axis=0)[:, :, :L, np.newaxis] # 2 x F x L x 1
            stft_masked = stft * mask
            wav_masked = self.compute_istft(stft_masked)

            wavs.append(wav_masked)

        return wavs


if __name__ == '__main__':
    sr = 44100
    es = MNN_Estimator()

    wav, _ = librosa.load('./coc.wav', mono=False, res_type='kaiser_fast', sr=sr)

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
        audio_data = wavs[i].T
        soundfile.write(fname, audio_data, sr, "PCM_16")

