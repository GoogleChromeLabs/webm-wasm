FROM trzeci/emscripten

RUN apt-get update && \
    apt-get install -qqy doxygen
