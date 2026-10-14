FROM ubuntu:22.04

# Avoid tzdata interactive prompt during cmake/g++ install
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    g++ \
    libcurl4-openssl-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

EXPOSE 45678
EXPOSE 8000

RUN chmod +x run.sh
CMD ["./run.sh"]
