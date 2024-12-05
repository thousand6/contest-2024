# FROM alpine:latest
# RUN mkdir -p /app/output
# RUN mkdir -p /app/src
# RUN apk add build-base
# COPY src/ /app/src
# WORKDIR /app/src
# RUN gcc contest.c rax.c main.c -lm -o ../output/myapp
# CMD ["time", "-f", "Time: %e s Memory: %M KB  CPU: %P\n", "/app/output/myapp"]

FROM harbor.vesync.com/contest/vesync-py311-base:1.0.0
RUN dnf install -y time
WORKDIR /app
COPY ./output /app
CMD ["/usr/bin/time", "-f", "Time: %e s Memory: %M KB  CPU: %P\n", "/app/myapp"]