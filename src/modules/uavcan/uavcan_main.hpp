/****************************************************************************
 *
 *   Copyright (c) 2014 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#pragma once

#include <nuttx/config.h>

#include <uavcan_stm32/uavcan_stm32.hpp>
#include <drivers/device/device.h>

#include <uORB/topics/actuator_controls.h>
#include <uORB/topics/actuator_outputs.h>
#include <uORB/topics/actuator_armed.h>

#include "actuators/esc.hpp"
#include "sensors/sensor_bridge.hpp"

/**
 * @file uavcan_main.hpp
 *
 * Defines basic functinality of UAVCAN node.
 *
 * @author Pavel Kirienko <pavel.kirienko@gmail.com>
 */

#define NUM_ACTUATOR_CONTROL_GROUPS_UAVCAN	4
#define UAVCAN_DEVICE_PATH	"/dev/uavcan/esc"

/**
 * A UAVCAN node.
 */
class UavcanNode : public device::CDev
{
	static constexpr unsigned MemPoolSize        = 10752;
	static constexpr unsigned RxQueueLenPerIface = 64;
	static constexpr unsigned StackSize          = 3000;

public:
	typedef uavcan::Node<MemPoolSize> Node;
	typedef uavcan_stm32::CanInitHelper<RxQueueLenPerIface> CanInitHelper;

	UavcanNode(uavcan::ICanDriver &can_driver, uavcan::ISystemClock &system_clock);

	virtual		~UavcanNode();

	virtual int	ioctl(file *filp, int cmd, unsigned long arg);

	static int	start(uavcan::NodeID node_id, uint32_t bitrate);

	Node&		get_node() { return _node; }

	// TODO: move the actuator mixing stuff into the ESC controller class
	static int	control_callback(uintptr_t handle, uint8_t control_group, uint8_t control_index, float &input);

	void		subscribe();

	int		teardown();
	int		arm_actuators(bool arm);

	void		print_info();

	static UavcanNode* instance() { return _instance; }

private:
	void		fill_node_info();
	int		init(uavcan::NodeID node_id);
	void		node_spin_once();
	int		run();

	int			_task = -1;			///< handle to the OS task
	bool			_task_should_exit = false;	///< flag to indicate to tear down the CAN driver
	int			_armed_sub = -1;		///< uORB subscription of the arming status
	actuator_armed_s	_armed;				///< the arming request of the system
	bool			_is_armed = false;		///< the arming status of the actuators on the bus

	unsigned		_output_count = 0;		///< number of actuators currently available

	static UavcanNode	*_instance;			///< singleton pointer
	Node			_node;				///< library instance
	pthread_mutex_t		_node_mutex;

	UavcanEscController	_esc_controller;

	List<IUavcanSensorBridge*> _sensor_bridges;		///< List of active sensor bridges

	MixerGroup		*_mixers = nullptr;

	uint32_t		_groups_required = 0;
	uint32_t		_groups_subscribed = 0;
	int			_control_subs[NUM_ACTUATOR_CONTROL_GROUPS_UAVCAN] = {};
	actuator_controls_s 	_controls[NUM_ACTUATOR_CONTROL_GROUPS_UAVCAN] = {};
	orb_id_t		_control_topics[NUM_ACTUATOR_CONTROL_GROUPS_UAVCAN] = {};
	pollfd			_poll_fds[NUM_ACTUATOR_CONTROL_GROUPS_UAVCAN + 1] = {};	///< +1 for /dev/uavcan/busevent
	unsigned		_poll_fds_num = 0;
};
