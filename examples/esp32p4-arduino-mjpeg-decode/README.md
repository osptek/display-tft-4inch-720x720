```
ffmpeg -i input.mp4 -vf "fps=30,scale=-1:720:flags=lanczos,crop=720:in_h:(in_w-720)/2:0" -q:v 9 mjpeg_720_720_30fps.mjpeg
```

# 注意事项

>+ 启动PSRAM

>+ 宽和高是16倍数