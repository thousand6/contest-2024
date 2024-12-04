FROM alpine:latest
RUN mkdir -p /app/output
RUN mkdir -p /app/src
RUN apk add build-base
COPY src/ /app/src
WORKDIR /app/src
RUN gcc contest.c rax.c main.c -lm -o ../output/myapp
CMD ["time", "-f", "Time: %e s Memory: %M KB  CPU: %P\n", "/app/output/myapp"]