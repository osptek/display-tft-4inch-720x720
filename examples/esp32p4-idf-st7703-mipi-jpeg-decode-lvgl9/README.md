```
ffmpeg -i input.jpg -vf "scale=720:720:force_original_aspect_ratio=decrease,pad=720:720:(ow-iw)/2:(oh-ih)/2:black" -pix_fmt yuvj420p -q:v 1 image_720_720.jpg
```

# 注意事项

>+ 启动PSRAM

>+ 宽和高是16倍数

# 支持长文件名
```
FATFS_LONG_FILENAMES=CONFIG_FATFS_LFN_HEAP
FATFS_MAX_LFN=255
```