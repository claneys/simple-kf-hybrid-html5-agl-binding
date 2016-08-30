/*
 * Copyright (C) 2015, 2016 "IoT.bzh"
 * Author "Manuel Bachmann"
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define _GNU_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <json-c/json.h>

#include <systemd/sd-event.h>

#include <afb/afb-binding.h>
#include <afb/afb-service-itf.h>

/***************************************************************************************/
/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: HANDLING OF CONNECTION                                             **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
/***************************************************************************************/
/* declare the connection routine */
static int connection();

/*
 * called on an event on the NMEA stream
 */
static int on_event(sd_event_source *s, int fd, uint32_t revents, void *userdata)
{
	/* read available data */
	if ((revents & EPOLLIN) != 0) {
		nmea_read(fd);
		event_send();
	}

	/* check if error or hangup */
	if ((revents & (EPOLLERR|EPOLLRDHUP|EPOLLHUP)) != 0) {
		sd_event_source_unref(s);
		close(fd);
		connection(fd);
	}

	return 0;
}

/*
 * opens a socket to a host and a service (or port)
 */
static int open_socket_to(const char *host, const char *service)
{
	int rc, fd;
	struct addrinfo hint, *rai, *iai;

	/* get addr */
	memset(&hint, 0, sizeof hint);
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;
	rc = getaddrinfo(host, service, &hint, &rai);
	if (rc != 0)
		return -1;

	/* get the socket */
	iai = rai;
	while (iai != NULL) {
		fd = socket(iai->ai_family, iai->ai_socktype, iai->ai_protocol);
		if (fd >= 0) {
			rc = connect(fd, iai->ai_addr, iai->ai_addrlen);
			if (rc == 0) {
				fcntl(fd, F_SETFL, O_NONBLOCK);
				freeaddrinfo(rai);
				return fd;
			}
			close(fd);
		}
		iai = iai->ai_next;
	}
	freeaddrinfo(rai);
	return -1;
}

/*
 * connection to nmea stream for the host and the port
 */
static int connect_to(const char *host, const char *service, int isgpsd)
{
	sd_event_source *source;
	int rc, fd;

	/* TODO connect to somewhere else */
	fd = open_socket_to(host, service);
	if (fd < 0) {
		ERROR(afbitf, "can't connect to host %s, service %s", host, service);
		return fd;
	}
	if (isgpsd) {
		static const char gpsdsetup[] = "?WATCH={\"enable\":true,\"nmea\":true};\r\n";
		write(fd, gpsdsetup, sizeof gpsdsetup - 1);
	}

	/* adds to the event loop */
	rc = sd_event_add_io(afb_daemon_get_event_loop(afbitf->daemon), &source, fd, EPOLLIN, on_event, NULL);
	if (rc < 0) {
		close(fd);
		ERROR(afbitf, "can't coonect host %s, service %s to the event loop", host, service);
	} else {
		NOTICE(afbitf, "Connected to host %s, service %s", host, service);
	}
	return rc;
}

/*
 * connection to nmea stream
 */
static int connection()
{
	const char *host;
	const char *service;
	int isgpsd;

	/* TODO connect to somewhere else */
	host = getenv("AFBGPS_HOST") ? : "sinagot.net";
	service = getenv("AFBGPS_SERVICE") ? : "5001";
	isgpsd = getenv("AFBGPS_ISNMEA") ? 0 : 1;
	return connect_to(host, service, isgpsd);
}

/***************************************************************************************/
/***************************************************************************************/
/**                                                                                   **/
/**                                                                                   **/
/**       SECTION: BINDING VERBS IMPLEMENTATION                                       **/
/**                                                                                   **/
/**                                                                                   **/
/***************************************************************************************/
/***************************************************************************************/
/*
 * Returns the type corresponding to the given name
 */
static enum type type_of_name(const char *name)
{
	enum type result;
	if (name == NULL)
		return type_DEFAULT;
	for (result = 0 ; result != type_COUNT ; result++)
		if (strcmp(type_NAMES[result], name) == 0)
			return result;
	return type_INVALID;
}

/*
 * extract a valid type from the request
 */
static int get_type_for_req(struct afb_req req, enum type *type)
{
	if ((*type = type_of_name(afb_req_value(req, "type"))) != type_INVALID)
		return 1;
	afb_req_fail(req, "unknown-type", NULL);
	return 0;
}
	
/*
 * Get the last known position
 *
 * parameter of the get are:
 *
 *    type:   string: the type of position expected (defaults to "WGS84" if not present)
 *
 * returns the position
 *
 * The valid types are:
 *
 *  +==========+=======================+=======+==========+=======+
 *  | type     |  longitude & latitude | speed | altitude | track |
 *  +==========+=======================+=======+==========+=======+
 *  | WGS84    |      degre            |  m/s  |          |       |
 *  +----------+-----------------------+-------+          |       |
 *  | DMS.km/h |                       | km/h  |          |       |
 *  +----------+                       +-------+   meter  | degre |
 *  | DMS.mph  |   deg°min'sec"X       |  mph  |          |       |
 *  +----------+                       +-------+          |       |
 *  | DMS.kn   |                       |  kn   |          |       |
 *  +==========+=======================+=======+==========+=======+
 */
static void get_gps(struct afb_req req)
{
	enum type type;
	if (get_type_for_req(req, &type))
		afb_req_success(req, position(type), NULL);
}

/*
 * subscribe to notification of position
 *
 * parameters of the subscription are:
 *
 *    type:   string:  the type of position expected (defaults to WCS84 if not present)
 *                     see the list above (get_gps)
 *    period: integer: the expected period in milliseconds (defaults to 2000 if not present)
 *
 * returns an object with 2 fields:
 *
 *    name:   string:  the name of the event without its prefix
 *    id:     integer: a numeric identifier of the event to be used for unsubscribing
 */
static void subscribe(struct afb_req req)
{
	enum type type;
	const char *period;
	struct event *event;
	struct json_object *json;

	if (get_type_for_req(req, &type)) {
		period = afb_req_value(req, "period");
		event = event_get(type, period == NULL ? DEFAULT_PERIOD : atoi(period));
		if (event == NULL)
			afb_req_fail(req, "out-of-memory", NULL);
		else if (afb_req_subscribe(req, event->event) != 0)
			afb_req_fail_f(req, "failed", "afb_req_subscribe returned an error: %m");
		else {
			json = json_object_new_object();
			json_object_object_add(json, "name", json_object_new_string(event->name));
			json_object_object_add(json, "id", json_object_new_int(event->id));
			afb_req_success(req, json, NULL);
		}
	}
}

/*
 * unsubscribe a previous subscription
 *
 * parameters of the unsubscription are:
 *
 *    id:   integer: the numeric identifier of the event as returned when subscribing
 */
static void unsubscribe(struct afb_req req)
{
	const char *id;
	struct event *event;

	id = afb_req_value(req, "id");
	if (id == NULL)
		afb_req_fail(req, "missing-id", NULL);
	else {
		event = event_of_id(atoi(id));
		if (event == NULL)
			afb_req_fail(req, "bad-id", NULL);
		else {
			afb_req_unsubscribe(req, event->event);
			afb_req_success(req, NULL, NULL);
		}
	}
}

/*
 * array of the verbs exported to afb-daemon
 */
static const struct afb_verb_desc_v1 binding_verbs[] = {
  /* VERB'S NAME            SESSION MANAGEMENT          FUNCTION TO CALL         SHORT DESCRIPTION */
  { .name= "ping"     , .session= AFB_SESSION_NONE, .callback= ping    , "Ping the binder"},
  { .name= "get_gyr"  , .session= AFB_SESSION_NONE, .callback= get_gyr , "Get Gyroscop values"},
  { .name= "get_acc"  , .session= AFB_SESSION_NONE, .callback= get_acc , "Get Accelerometer values"},
  { .name= "get_mag"  , .session= AFB_SESSION_NONE, .callback= get_mag , "Get Magnetometer values"},
  { .name= "get_gps"  , .session= AFB_SESSION_NONE, .callback= get_gps , "Get last GPS values"},
  { .name= "subscribe",    .session= AFB_SESSION_NONE, .callback= subscribe,    .info= "subscribe to notification of position" },
  { .name= "unsubscribe",  .session= AFB_SESSION_NONE, .callback= unsubscribe,  .info= "unsubscribe a previous subscription" },
  { .name= NULL } /* marker for end of the array */
};

/*
 * description of the binding for afb-daemon
 */
static const struct afb_binding binding_description =
{
  /* description conforms to VERSION 1 */
  .type= AFB_BINDING_VERSION_1,
  .v1= {			/* fills the v1 field of the union when AFB_BINDING_VERSION_1 */
    .info = "GPS+, providing enhanced GPS position, hybrid service",
    .prefix = "gps+",
    .verbs = binding_verbs	/* the array describing the verbs of the API */
  }
};

/*
 * activation function for registering the binding called by afb-daemon
 */
const struct afb_binding *afbBindingV1Register(const struct afb_binding_interface *itf)
{
	afbitf = itf;			/* records the interface for accessing afb-daemon */
	return &binding_description;	/* returns the description of the binding */
}

int afbBindingV1ServiceInit(struct afb_service service)
{
	return connection();
}
const struct afb_binding_interface *interface;

static void ping (struct afb_req request)
{
	static int pingcount = 0;

	json_object *query = afb_req_json(request);
	afb_req_success_f(request, NULL, "Ping Binder Daemon count=%d query=%s", ++pingcount, json_object_to_json_string(query));
}

