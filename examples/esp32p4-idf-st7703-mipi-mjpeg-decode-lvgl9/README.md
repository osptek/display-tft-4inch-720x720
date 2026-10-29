```
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:720:flags=lanczos,crop=720:in_h:(in_w-720)/2:0" -q:v 5 mjpeg_720_720_30fps.mjpeg
```

# 注意事项

>+ 启动PSRAM

>+ 宽和高是16倍数

# 支持长文件名
```
FATFS_LONG_FILENAMES=CONFIG_FATFS_LFN_HEAP
FATFS_MAX_LFN=255
```