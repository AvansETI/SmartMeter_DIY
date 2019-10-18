/*
 * Smart meter header
 * 
 * @author Michel Megens
 * @email  michel@michelmegens.net
 */

#pragma once

#include <Arduino.h>

/* Application configuration */
#define MQTT_USERNAME_LENGTH       32
#define MQTT_PASSWORD_LENGTH       32
#define MQTT_ID_TOKEN_LENGTH       64
#define MQTT_TOPIC_STRING_LENGTH   64
#define MQTT_REMOTE_HOST_LENGTH   128
#define MQTT_REMOTE_PORT_LENGTH    10

typedef struct {
   char     mqtt_username[MQTT_USERNAME_LENGTH];
   char     mqtt_password[MQTT_PASSWORD_LENGTH];
   char     mqtt_id[MQTT_ID_TOKEN_LENGTH];
   char     mqtt_topic[MQTT_TOPIC_STRING_LENGTH];
   char     mqtt_remote_host[MQTT_REMOTE_HOST_LENGTH];
   char     mqtt_remote_port[MQTT_REMOTE_HOST_LENGTH];
} APP_CONFIG_STRUCT;

typedef enum { EV_P1_AVAILABLE, EV_IDLE } ENUM_EVENT;
typedef enum { STATE_START, STATE_IDLE, STATE_MQTT } ENUM_STATE;

typedef struct {
   void (*pre)(void);
   void (*heartbeat)(void);
   void (*post)(void);
   ENUM_STATE nextState;
} STATE_TRANSITION_STRUCT;

/* RGB definitions */
typedef enum {
  RED = 0, GREEN, BLUE
} RGB_COLOR_ENUM;

typedef enum {
  OFF = 0, ON
} RGB_STATE_ENUM;

/* P1 statemachine */
typedef enum { 
   P1_MSG_S0,
   P1_MSG_S1,
   P1_MSG_S2
} ENUM_P1_MSG_STATE;

#define P1_MAX_DATAGRAM_SIZE 1024
#define P1_TELEGRAM_SIZE   1024

typedef struct {
   char p1_telegram[P1_TELEGRAM_SIZE];
} MEASUREMENT_STRUCT;

/* Prototype FSM functions. */
