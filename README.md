# CPP trading engine
My personal trading engine project, using C++, I build every core features in this project including: **Json** class, **coroutine model**, **https server** + **API system**, **mongoDB** integration, ....


# Build command (Linux)
```
cd <project-folder>
mkdir build
cd build/
cmake ..
make -j
```

# Run command  (Linux)
```
chmod 777 http_server.out
cp http_server.out ../
cd ..
./http_server.out 8080 web_data
```
