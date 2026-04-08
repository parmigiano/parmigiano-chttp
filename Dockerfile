FROM kalilinux/kali-rolling AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential bash curl git sudo \
    postgresql-server-dev-all libmaxminddb-dev \
    libhiredis-dev libcurl4-openssl-dev libssl-dev \
    libargon2-dev uuid-dev libcjson-dev pkg-config \
    ca-certificates wget zip \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/local/src

RUN git clone https://github.com/bji/libs3.git \
    && cd libs3 && make && make install

RUN cp /usr/local/src/libs3/build/lib/libs3.so.4 /usr/local/lib/ \
    && ln -sf /usr/local/lib/libs3.so.4 /usr/local/lib/libs3.so

RUN curl -s https://raw.githubusercontent.com/netcorelink/libchttpx/main/scripts/install.sh | sh

# Install mmdm
RUN wget https://github.com/P3TERX/GeoLite.mmdb/releases/latest/download/GeoLite2-Country.mmdb \
    && mkdir -p /usr/local/share/GeoIP \
    && cp GeoLite2-Country.mmdb /usr/local/share/GeoIP/

WORKDIR /app

COPY . .

RUN make clean && make TARGET=server-http lin

FROM kalilinux/kali-rolling

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 libcurl4 libssl3 libargon2-1 \
    uuid-runtime ca-certificates libmaxminddb0 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/lib/x86_64-linux-gnu/libhiredis*.so* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libcjson*.so* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/lib/x86_64-linux-gnu/libxml2*.so* /usr/lib/x86_64-linux-gnu/
COPY --from=builder /usr/local/src/libs3*.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/*.so* /usr/local/lib/
COPY --from=builder /usr/local/share/GeoIP/GeoLite2-Country.mmdb /usr/local/share/GeoIP/
COPY --from=builder /app/.build/server-http .
COPY --from=builder /app/docs ./docs
COPY --from=builder /app/src/infra ./src/infra

RUN ldconfig

ENV LD_LIBRARY_PATH=/usr/local/lib

EXPOSE 8080

CMD ["./server-http"]
