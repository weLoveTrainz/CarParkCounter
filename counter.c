#include <wiringPi.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

// Sensor constants
#define sensorPin 0
#define trigPin 4
#define echoPin 5
#define MAX_DISTANCE 220          // define the maximum measured distance
#define timeOut MAX_DISTANCE * 60 // calculate timeout according to the maximum measured distance
#define defaultDistance 78.57

// TCP constants
#define PORT 8080          /* the port client will be connecting to */
#define SERVER "127.0.0.1" /* replace this with your server IP address*/

// All elements are by default 0
int movementRecord[2048] = {0};
int distanceRecord[2048] = {0};
int endOfMovementRecordIndex = 5;
int endOfDistanceRecordIndex = 5;
int numberOfCars = 0;

int pulseIn(int pin, int level, int timeout);

float getSonar()
{
    long pingTime;
    float distance;
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    pingTime = pulseIn(echoPin, HIGH, timeOut);         //read plus time of echoPin
    distance = (float)pingTime * 340.0 / 2.0 / 10000.0; //calculate distance with sound speed 340m/s
    return distance;
}

bool hasDistanceChanged(int distance)
{
    return (defaultDistance - distance) > 10;
}

bool isThereMovement()
{
    return (digitalRead(sensorPin) == HIGH);
}

bool trueInLastFive(int index, int record[])
{
    for (int i = (index - 5); i < index; i++)
    {
        if (record[i] == 1)
        {
            return true;
        }
    }
    return false;
}

void clearLastFive(int index, int record[])
{
    for (int i = (index - 5); i < index; i++)
    {
        record[i] = 0;
    }
}

int main()
{
    printf("Program is starting ... \n");

    // Sensor variables
    wiringPiSetup();

    bool isMovement;
    bool isDistanceChange;
    float distance = 0;
    char str[20];
    pinMode(trigPin, OUTPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(sensorPin, INPUT);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // TCP variables
    int sockfd;
    struct sockaddr_in server;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr(SERVER);
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    // This is typecasting the address of the server struct to a a pointer to a sockaddr
    if (connect(sockfd, (struct sockaddr *)&server,
                sizeof(struct sockaddr)) < 0)

    {
        perror("connect");
        exit(1);
    }

    while (1)
    {
        distance = getSonar();
        isMovement = isThereMovement();
        isDistanceChange = hasDistanceChanged(distance);

        if (isMovement)
        {
            printf("movement detected\n");
            if (trueInLastFive(endOfDistanceRecordIndex, distanceRecord))
            {
                clearLastFive(endOfDistanceRecordIndex, distanceRecord);
                numberOfCars++;
            }
            else
            {
                movementRecord[endOfMovementRecordIndex] = 1;
            }
        }
        else if (isDistanceChange)
        {
            if (trueInLastFive(endOfMovementRecordIndex, movementRecord))
            {
                clearLastFive(endOfMovementRecordIndex, movementRecord);
                if (numberOfCars != 0)
                {
                    numberOfCars--;
                }
            }
            else
            {
                distanceRecord[endOfDistanceRecordIndex] = 1;
            }
        }
        printf("Number of cars in the carpark is %d\n", numberOfCars);
        sprintf(str, "%i", numberOfCars);

        printf("Sending to server: %d\n");
        send(sockfd, str, strlen(str), 0);

        endOfDistanceRecordIndex++;
        endOfMovementRecordIndex++;

        delay(1000);
    }

    close(sockfd);

    return 1;
}

// This code below came included with the sensor
int pulseIn(int pin, int level, int timeout)
{
    struct timeval tn, t0, t1;
    long micros;
    gettimeofday(&t0, NULL);
    micros = 0;
    while (digitalRead(pin) != level)
    {
        gettimeofday(&tn, NULL);
        if (tn.tv_sec > t0.tv_sec)
            micros = 1000000L;
        else
            micros = 0;
        micros += (tn.tv_usec - t0.tv_usec);
        if (micros > timeout)
            return 0;
    }
    gettimeofday(&t1, NULL);
    while (digitalRead(pin) == level)
    {
        gettimeofday(&tn, NULL);
        if (tn.tv_sec > t0.tv_sec)
            micros = 1000000L;
        else
            micros = 0;
        micros = micros + (tn.tv_usec - t0.tv_usec);
        if (micros > timeout)
            return 0;
    }
    if (tn.tv_sec > t1.tv_sec)
        micros = 1000000L;
    else
        micros = 0;
    micros = micros + (tn.tv_usec - t1.tv_usec);
    return micros;
}