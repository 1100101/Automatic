FROM alpine:latest
MAINTAINER x3m444
ENV TZ='Europe/Bucharest'
RUN apk update
RUN apk --no-cache add autoconf build-base automake git curl-dev curl pcre pcre-dev libxml2-dev libxml2 make
RUN mkdir /app/
RUN cd /app
RUN git clone https://github.com/1100101/Automatic.git /app
WORKDIR /app
RUN ./autogen.sh
RUN ./configure
RUN make&make install
RUN mv src/automatic.conf-sample /etc/automatic.conf
RUN mkdir -p /app/config
RUN touch /app/config/automatic.state
VOLUME /config
ENTRYPOINT /usr/local/bin/automatic -f
