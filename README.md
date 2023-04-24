# Share Shift

Other Language: [简体中文](./docs/zh-CN/README.zh-CN.md)

![Share Shift](docs/res/shareshif_logo.svg)



A cloud share platform with fastdfs and nginx

<p style="background-color:deepskyblue;color:red;">
Warning: This project is still under development, and the code is not buildable.</p>
<p style="background-color:deepskyblue;color:red;">
Can't run anyway!!
</p>

## Install

first, you need to build the enviroment, see [the enviroment file](./docs/enviroment.md), then you can install the project as followed:

```bash
    # install
    mkdir build
    cd build
    cmake ..
    make
```

## Usage

```bash
    cd ShareShift
    cp share_shift.conf ShareShift/build
    tmux
    ./share_shift
```
