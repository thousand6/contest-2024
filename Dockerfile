FROM harbor.vesync.com/contest/vesync-java-base:1.12
RUN dnf install -y time
COPY ./output /app
WORKDIR /app
CMD ["/usr/bin/time", "-f", "Time: %e s Memory: %M KB  CPU: %P\n", "./myapp"]