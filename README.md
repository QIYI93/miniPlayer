# MiniPlayer

## 基于ffmpeg的高性能音视频播放项目(仅用于学习)

1.使用ffmpe提供的解码方案.

2.参考微软及ffmpeg的DXVA2解码方案实现在部分特定编码下实现硬件加速

https://msdn.microsoft.com/en-us/library/windows/desktop/cc307941(v=vs.85).aspx

https://trac.ffmpeg.org/wiki/HWAccelIntro#DXVA2

3.音频播放部分使用XAudio2 API

4.视频渲染部分用了基于SDL和D3D9两种框架分别实现（SDL时不支持DXVA2解码）


## Build MiniPlayer

git clone https://github.com/QIYI93/miniPlayer.git

git submodule update --init

使用CMake生成VS工程(仅支持vs2015或更高版本)
