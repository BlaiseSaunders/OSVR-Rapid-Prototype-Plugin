// Copyright 2016 Sensics Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Internal Includes
#include <osvr/PluginKit/PluginKit.h>
#include <osvr/PluginKit/TrackerInterfaceC.h>

// Generated JSON header file
#include "org_osvr_DIYTracker_Tracker_json.h"

// Library/third-party includes

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

// Standard includes
#include <chrono>
#include <winsock2.h>
#include <stdio.h>
#include <thread>
#include <string.h>

#define BUFLEN 512
#define PORT 5005

#pragma comment(lib,"ws2_32.lib") //Winsock Library

// Anonymous namespace to avoid symbol collision
namespace
{

class TrackerDevice
{
  public:

	TrackerDevice(OSVR_PluginRegContext ctx)
	{


        /// Create the initialization options
        OSVR_DeviceInitOptions opts = osvrDeviceCreateInitOptions(ctx);

        // Configure our tracker
        osvrDeviceTrackerConfigure(opts, &m_tracker);

        /// Create the device token with the options
        m_dev.initAsync(ctx, "Tracker", opts);

        /// Send JSON descriptor
        m_dev.sendJsonDescriptor(org_osvr_DIYTracker_Tracker_json);

        /// Register update callback
        m_dev.registerUpdateCallback(this);

		slen = sizeof(si_other);

		// Initialise winsock
		printf("\nInitialising Winsock...");
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		{
			printf("Failed. Error Code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		printf("Initialised.\n");

		// Make socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			printf("Failed to create socket : %d", WSAGetLastError());

		// Set the socket's options
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(PORT);

		if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
		{
			printf("Bind failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
    }

    OSVR_ReturnCode update()
    {
		float points[3];

        std::this_thread::sleep_for(std::chrono::milliseconds(
            5)); // Simulate waiting a quarter second for data.

        OSVR_TimeValue now;
        osvrTimeValueGetNow(&now);


		memset(buf, '\0', BUFLEN);

		//
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("Recieving data failed with error: %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		printf("Raw packet: %s\n", buf);

		points[0] = atof(strtok(buf, ","));
		for (int i = 1; i < 3; i++)
			points[i] = atof(strtok(NULL, ","));

		printf("POINTS: %.2f,%.2f,%.2f\n", points[0], points[1], points[2]);

        /// Report the identity pose for sensor 0
        OSVR_PoseState pose;
        osvrPose3SetIdentity(&pose);
        pose.translation.data[0] = points[0];
        pose.translation.data[1] = points[1];
        pose.translation.data[2] = points[2];
        osvrDeviceTrackerSendPose(m_dev, m_tracker, &pose, 0);

        return OSVR_RETURN_SUCCESS;
    }


  private:
    osvr::pluginkit::DeviceToken m_dev;
    OSVR_TrackerDeviceInterface m_tracker;
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_len;
	char buf[BUFLEN];
	WSADATA wsa;

};

class HardwareDetection {
  public:
    HardwareDetection() : m_found(false) {}
    OSVR_ReturnCode operator()(OSVR_PluginRegContext ctx) {

        if (m_found) {
            return OSVR_RETURN_SUCCESS;
        }

        /// Update later...
        m_found = true;

        /// Create our device object
        osvr::pluginkit::registerObjectForDeletion(ctx, new TrackerDevice(ctx));

        return OSVR_RETURN_SUCCESS;
    }

  private:
    bool m_found;
};
} // namespace

OSVR_PLUGIN(org_osvr_icecream_Tracker) {

    osvr::pluginkit::PluginContext context(ctx);

    /// Register a detection callback function object.
    context.registerHardwareDetectCallback(new HardwareDetection());

    return OSVR_RETURN_SUCCESS;
}
