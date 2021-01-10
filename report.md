## 环境配置
```bash
# ubuntu 20.04
sudo apt install qemu
sudo apt install qemu-system-i386
```



## 运行

```bash
make
make qemu
```



## 功能支持
- [x] 光标及鼠标使用
- [x] 使用上下左右键切换光标位置
- [x] 使用Home，End键回到行首行尾
- [x] 使用Ctrl-Home回到文件最开始位置
- [x] 使用Ctrl-End回到文件最结尾位置
- [x] 使用Ctrl-C复制，Ctrl-V 粘贴，Ctrl-X 剪切
- [x] 使用Shift加上下左右、Home、End键进行自由选择
- [x] 使用Ctrl-A全选
- [x] 支持c语言的自动高亮显示
- [x] 使用Ctrl-Z，Ctrl-Shift-Z进行撤销和反撤销
- [x] 使用Ctrl-S保存文件
- [x] 使用Ctrl-F进行搜索和替换，使用Tab和Shift+Tab可以在搜索框和替换框中切换，使用Enter键进行替换操作，使用Esc键退出
- [x] 使用Ctrl-+/-，放大或缩小字体大小
- [x] 使用Ctrl-`调出内嵌终端，可以在里面执行命令，使用Esc键退出



## 模块架构

![](graph.png)

鼠标驱动和键盘驱动通过中断生成对应消息，置入消息队列后，分发到程序（文本编辑器）进行处理，通过调用GUI绘制引擎更新界面



