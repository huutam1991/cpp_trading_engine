FROM ubuntu:22.04

# Install libraries
WORKDIR /install_folder
ADD install_library.sh /install_folder
RUN chmod 777 install_library.sh
RUN ./install_library.sh

# Copy source code into container
WORKDIR /cpp_trading_engine
ADD core /cpp_trading_engine/core
ADD server_app /cpp_trading_engine/server_app
ADD web_data /cpp_trading_engine/web_data
ADD z_util_scripts /cpp_trading_engine/z_util_scripts
ADD build_bash.sh /cpp_trading_engine
ADD CMakeLists.txt /cpp_trading_engine
ADD run_bash.sh /cpp_trading_engine

# Add lib to system path
#ENV ARM64="True"
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Build source code
RUN chmod 777 build_bash.sh
RUN chmod 777 run_bash.sh
RUN ./build_bash.sh

# tell the port number the container should expose
EXPOSE 443 80 8081

# run the command
CMD ["./http_server_cpp", "443", "web_data"]
