# spleeter-mnn
Spleeter implementation in MNN.

## Usage
### Python
See `python/test_mnn_estimator.py` for the usage (`python/test_estimator.py` is an another implementation in PyTorch for the original model).
Use script `python/convert2onnx.py` and MNN's convert tool (provided you have already compiled and installed MNN correctly)
```
./MNNConvert -f ONNX --modelFile XXX.onnx --MNNModel XXX.mnn --bizCode biz
```
to convert the original model in `checkpoints/2stems` to the MNN model.

### C++
C++ implementation is an another transcript of Python implementation. It uses `Eigen::Tensor` to replace the `ndarray`.
```
sh test.sh
```

## Note

* I only tested with 2stems model, not sure if it works for other models.
* There might be some bugs, the quality of output isn't as good as origin. See `python/output` for some results. If you find the reason, please send me a merge request. Thanks.

## Acknowledge
* [Spleeter](https://github.com/deezer/spleeter)
* [MNN](https://www.yuque.com/mnn)
* [spleeter-pytorch](https://github.com/tuan3w/spleeter-pytorch) Model architecture in `spleeter/unet.py`
* [spleeter-pytorch-mnn](https://github.com/bigcash/spleeter-pytorch-mnn) Mainly inspired by this repo
