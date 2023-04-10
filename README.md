# Share Shift

[toc]

build a cloud share platform with fastdfs and nginx



## Install

first, you need to build the enviroment, see [the enviroment file](./docs/enviroment.md), then you can install the project as follow:

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

## Project Structure

```mermaid
graph LR
    subgraph Browser
    B((Browser))
    end
    subgraph Server
    N[Nginx]
    S[Server]
    M[MySQL]
    R[Redis]
    F[fastdfs]
    T((Temp Directory))
    end
    B ---|80| N
    N ---|API| S
    N ---|Upload file to temp directory| T
    T ---|File transfer using pipe| F
    S ---|Download file to send to Browser| T
    S ---|Exchange data| M
    S ---|Exchange data| R

```