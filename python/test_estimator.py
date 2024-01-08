import numpy as np
import librosa
import soundfile
import torch
import os
from spleeter.estimator import Estimator

sub_dir = os.path.dirname(os.path.abspath(__file__))


if __name__ == '__main__':
    sr = 44100
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    es = Estimator(2, '../checkpoints/2stems/model').to(device)
    es.eval()

    # load wav audio
    wav, _ = librosa.load('../audio/coc.wav', mono=False, res_type='kaiser_fast', sr=sr)
    wav = torch.Tensor(wav).to(device)

    # normalize audio
    # wav_torch = wav / (wav.max() + 1e-8)
    output_dir = os.path.join(sub_dir, 'output')
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    wavs = es.separate(wav)
    for i in range(len(wavs)):
        fname = os.path.join(sub_dir, 'output/out_{}.wav').format(i)
        print('Writing ',fname)
        soundfile.write(fname, wavs[i].cpu().detach().numpy().T, sr, "PCM_16")
        # write_wav(fname, np.asfortranarray(wavs[i].squeeze().numpy()), sr)
