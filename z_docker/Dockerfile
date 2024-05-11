FROM ubuntu:22.04

# Install libraries
WORKDIR /install_folder
ADD ../install_library.sh /install_folder
RUN chmod 777 install_library.sh
RUN ./install_library.sh

# Copy source code into container
WORKDIR /manualtrader
ADD core /manualtrader/core
ADD server_app /manualtrader/server_app
ADD web_data /manualtrader/web_data
ADD build_bash.sh /manualtrader
ADD CMakeLists.txt /manualtrader
ADD run_bash.sh /manualtrader
ADD server-certificate.crt /manualtrader
ADD server-private-key.pem /manualtrader

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
