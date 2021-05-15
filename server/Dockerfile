FROM alpine:3.13 AS build

RUN apk update && apk add --no-cache alpine-sdk cmake boost-dev boost-static

WORKDIR /usr/local/src/fouracross

COPY . .

WORKDIR /usr/local/src/fouracross/build

RUN cmake .. 
RUN cmake --build . --target game server runserver

FROM alpine:3.13

RUN apk update && apk add --no-cache libstdc++ icu icu-libs

WORKDIR /usr/local/server

COPY --from=build /usr/local/src/fouracross/build/server/runserver .

EXPOSE 31001

CMD ["./runserver", "31001"]