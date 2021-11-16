# C Chat App

This is a socket app built all in C that allows you to connect between multiple users and send messages live to each other.

# How to Build

To build this project, in your linux terminal, run `make`. This has been developed to work on Ubuntu 20.04.3 LTS

![](./docs/imgs/specs.png)

In the root directory run `make` to build.

![](./docs/imgs/make.png)

# How to Run

## Run Executables Directly

Once you have it build you must first run the server executable by doing

```bash
./server 4000
```

This will open a port on 4000, you can optionally run the server in `debug mode` by adding the flag `DEBUG` after the server port

```bash
./server 4000 DEBUG
```

## Running Server for Development

An all in one build script is added to this project that will build both client and server executables, and allow you to run immediately. 

To use this script, run this in the terminal

```bash
./dev-server.sh
```

It should look like so:

![](./docs/imgs/dev-server.png)

Select `Y` will autostart the server on port 4000

# More Information

The `docs/` folder that is included in this repository contains architectural information about the project. Here are links to each piece of documentation

[Networking Architecture](./docs/architecture/networking.md)

[Extending The Code](./docs/architecture/continued-development.md)