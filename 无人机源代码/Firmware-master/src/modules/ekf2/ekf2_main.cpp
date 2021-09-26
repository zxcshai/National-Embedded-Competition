/****************************************************************************
 *
 *   Copyright (c) 2015 PX4 Development Team. All rights reserved.
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

/**
 * @file ekf2_main.cpp
 * Implementation of the attitude and position estimator.
 *
 * @author Roman Bapst
 */

#include <px4_config.h>
#include <px4_defines.h>
#include <px4_tasks.h>
#include <px4_posix.h>
#include <px4_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <time.h>
#include <float.h>

#include <arch/board/board.h>
#include <systemlib/param/param.h>
#include <systemlib/err.h>
#include <systemlib/systemlib.h>
#include <systemlib/mavlink_log.h>
#include <mathlib/mathlib.h>
#include <mathlib/math/filter/LowPassFilter2p.hpp>
#include <platforms/px4_defines.h>
#include <drivers/drv_hrt.h>
#include <controllib/uorb/blocks.hpp>

#include <uORB/topics/sensor_combined.h>
#include <uORB/topics/vehicle_gps_position.h>
#include <uORB/topics/airspeed.h>
#include <uORB/topics/vehicle_attitude.h>
#include <uORB/topics/vehicle_local_position.h>
#include <uORB/topics/control_state.h>
#include <uORB/topics/vehicle_global_position.h>
#include <uORB/topics/wind_estimate.h>
#include <uORB/topics/estimator_status.h>
#include <uORB/topics/ekf2_innovations.h>
#include <uORB/topics/actuator_armed.h>
#include <uORB/topics/ekf2_replay.h>
#include <uORB/topics/optical_flow.h>
#include <uORB/topics/distance_sensor.h>
#include <uORB/topics/vehicle_land_detected.h>

#include <ecl/EKF/ekf.h>


extern "C" __EXPORT int ekf2_main(int argc, char *argv[]);


class Ekf2;

namespace ekf2
{
Ekf2 *instance = nullptr;
}


class Ekf2 : public control::SuperBlock
{
public:
	/**
	 * Constructor
	 */
	Ekf2();

	/**
	 * Destructor, also kills task.
	 */
	~Ekf2();

	/**
	 * Start task.
	 *
	 * @return		OK on success.
	 */
	int		start();

	void 	set_replay_mode(bool replay) {_replay_mode = true;};

	static void	task_main_trampoline(int argc, char *argv[]);

	void		task_main();

	void print_status();

	void exit() { _task_should_exit = true; }

private:
	static constexpr float _dt_max = 0.02;
	bool		_task_should_exit = false;
	int		_control_task = -1;			// task handle for task
	bool 	_replay_mode;	// should we use replay data from a log
	int 	_publish_replay_mode;	// defines if we should publish replay messages

	int		_sensors_sub = -1;
	int		_gps_sub = -1;
	int		_airspeed_sub = -1;
	int		_params_sub = -1;
	int 	_optical_flow_sub = -1;
	int 	_range_finder_sub = -1;
	int		_actuator_armed_sub = -1;
	int		_vehicle_land_detected_sub = -1;

	bool            _prev_motors_armed = false; // motors armed status from the previous frame

	float _acc_hor_filt = 0.0f; 	// low-pass filtered horizontal acceleration

	orb_advert_t _att_pub;
	orb_advert_t _lpos_pub;
	orb_advert_t _control_state_pub;
	orb_advert_t _vehicle_global_position_pub;
	orb_advert_t _wind_pub;
	orb_advert_t _estimator_status_pub;
	orb_advert_t _estimator_innovations_pub;
	orb_advert_t _replay_pub;

	/* Low pass filter for attitude rates */
	math::LowPassFilter2p _lp_roll_rate;
	math::LowPassFilter2p _lp_pitch_rate;
	math::LowPassFilter2p _lp_yaw_rate;

	Ekf _ekf;

	parameters *_params;	// pointer to ekf parameter struct (located in _ekf class instance)

	control::BlockParamFloat _mag_delay_ms;
	control::BlockParamFloat _baro_delay_ms;
	control::BlockParamFloat _gps_delay_ms;
	control::BlockParamFloat _flow_delay_ms;
	control::BlockParamFloat _rng_delay_ms;
	control::BlockParamFloat _airspeed_delay_ms;

	control::BlockParamFloat _gyro_noise;
	control::BlockParamFloat _accel_noise;

	// process noise
	control::BlockParamFloat _gyro_bias_p_noise;
	control::BlockParamFloat _accel_bias_p_noise;
	control::BlockParamFloat _gyro_scale_p_noise;
	control::BlockParamFloat _mage_p_noise;
	control::BlockParamFloat _magb_p_noise;
	control::BlockParamFloat _wind_vel_p_noise;
	control::BlockParamFloat _terrain_p_noise;	// terrain offset state random walk (m/s)
	control::BlockParamFloat _terrain_gradient;	// magnitude of terrain gradient (m/m)

	control::BlockParamFloat _gps_vel_noise;
	control::BlockParamFloat _gps_pos_noise;
	control::BlockParamFloat _pos_noaid_noise;
	control::BlockParamFloat _baro_noise;
	control::BlockParamFloat _baro_innov_gate;     // innovation gate for barometric height innovation test (std dev)
	control::BlockParamFloat _posNE_innov_gate;    // innovation gate for GPS horizontal position innovation test (std dev)
	control::BlockParamFloat _vel_innov_gate;      // innovation gate for GPS velocity innovation test (std dev)

	control::BlockParamFloat _mag_heading_noise;	// measurement noise used for simple heading fusion
	control::BlockParamFloat _mag_noise;           // measurement noise used for 3-axis magnetoemter fusion (Gauss)
	control::BlockParamFloat _eas_noise;			// measurement noise used for airspeed fusion (std m/s)
	control::BlockParamFloat _mag_declination_deg;	// magnetic declination in degrees
	control::BlockParamFloat _heading_innov_gate;	// innovation gate for heading innovation test
	control::BlockParamFloat _mag_innov_gate;	// innovation gate for magnetometer innovation test
	control::BlockParamInt
	_mag_decl_source;       // bitmasked integer used to control the handling of magnetic declination
	control::BlockParamInt _mag_fuse_type;         // integer ued to control the type of magnetometer fusion used

	control::BlockParamInt _gps_check_mask;        // bitmasked integer used to activate the different GPS quality checks
	control::BlockParamFloat _requiredEph;         // maximum acceptable horiz position error (m)
	control::BlockParamFloat _requiredEpv;         // maximum acceptable vert position error (m)
	control::BlockParamFloat _requiredSacc;        // maximum acceptable speed error (m/s)
	control::BlockParamInt _requiredNsats;         // minimum acceptable satellite count
	control::BlockParamFloat _requiredGDoP;        // maximum acceptable geometric dilution of precision
	control::BlockParamFloat _requiredHdrift;      // maximum acceptable horizontal drift speed (m/s)
	control::BlockParamFloat _requiredVdrift;      // maximum acceptable vertical drift speed (m/s)
	control::BlockParamInt _param_record_replay_msg; // indicates if we want to record ekf2 replay messages

	// measurement source control
	control::BlockParamInt
	_fusion_mode;		// bitmasked integer that selects which of the GPS and optical flow aiding sources will be used
	control::BlockParamInt _vdist_sensor_type;	// selects the primary source for height data

	// range finder fusion
	control::BlockParamFloat _range_noise;		// observation noise for range finder measurements (m)
	control::BlockParamFloat _range_innov_gate;	// range finder fusion innovation consistency gate size (STD)
	control::BlockParamFloat _rng_gnd_clearance;	// minimum valid value for range when on ground (m)

	// optical flow fusion
	control::BlockParamFloat
	_flow_noise;		// best quality observation noise for optical flow LOS rate measurements (rad/sec)
	control::BlockParamFloat
	_flow_noise_qual_min;	// worst quality observation noise for optical flow LOS rate measurements (rad/sec)
	control::BlockParamInt _flow_qual_min;		// minimum acceptable quality integer from  the flow sensor
	control::BlockParamFloat _flow_innov_gate;	// optical flow fusion innovation consistency gate size (STD)
	control::BlockParamFloat _flow_rate_max;	// maximum valid optical flow rate (rad/sec)

	// sensor positions in body frame
	control::BlockParamFloat _imu_pos_x;	// X position of IMU in body frame (m)
	control::BlockParamFloat _imu_pos_y;	// Y position of IMU in body frame (m)
	control::BlockParamFloat _imu_pos_z;	// Z position of IMU in body frame (m)
	control::BlockParamFloat _gps_pos_x;	// X position of GPS antenna in body frame (m)
	control::BlockParamFloat _gps_pos_y;	// Y position of GPS antenna in body frame (m)
	control::BlockParamFloat _gps_pos_z;	// Z position of GPS antenna in body frame (m)
	control::BlockParamFloat _rng_pos_x;	// X position of range finder in body frame (m)
	control::BlockParamFloat _rng_pos_y;	// Y position of range finder in body frame (m)
	control::BlockParamFloat _rng_pos_z;	// Z position of range finder in body frame (m)
	control::BlockParamFloat _flow_pos_x;	// X position of optical flow sensor focal point in body frame (m)
	control::BlockParamFloat _flow_pos_y;	// Y position of optical flow sensor focal point in body frame (m)
	control::BlockParamFloat _flow_pos_z;	// Z position of optical flow sensor focal point in body frame (m)

	int update_subscriptions();

};

Ekf2::Ekf2():
	SuperBlock(NULL, "EKF"),
	_replay_mode(false),
	_publish_replay_mode(0),
	_att_pub(nullptr),
	_lpos_pub(nullptr),
	_control_state_pub(nullptr),
	_vehicle_global_position_pub(nullptr),
	_wind_pub(nullptr),
	_estimator_status_pub(nullptr),
	_estimator_innovations_pub(nullptr),
	_replay_pub(nullptr),
	_lp_roll_rate(250.0f, 30.0f),
	_lp_pitch_rate(250.0f, 30.0f),
	_lp_yaw_rate(250.0f, 20.0f),
	_ekf(),
	_params(_ekf.getParamHandle()),
	_mag_delay_ms(this, "EKF2_MAG_DELAY", false, &_params->mag_delay_ms),
	_baro_delay_ms(this, "EKF2_BARO_DELAY", false, &_params->baro_delay_ms),
	_gps_delay_ms(this, "EKF2_GPS_DELAY", false, &_params->gps_delay_ms),
	_flow_delay_ms(this, "EKF2_OF_DELAY", false, &_params->flow_delay_ms),
	_rng_delay_ms(this, "EKF2_RNG_DELAY", false, &_params->range_delay_ms),
	_airspeed_delay_ms(this, "EKF2_ASP_DELAY", false, &_params->airspeed_delay_ms),
	_gyro_noise(this, "EKF2_GYR_NOISE", false, &_params->gyro_noise),
	_accel_noise(this, "EKF2_ACC_NOISE", false, &_params->accel_noise),
	_gyro_bias_p_noise(this, "EKF2_GYR_B_NOISE", false, &_params->gyro_bias_p_noise),
	_accel_bias_p_noise(this, "EKF2_ACC_B_NOISE", false, &_params->accel_bias_p_noise),
	_gyro_scale_p_noise(this, "EKF2_GYR_S_NOISE", false, &_params->gyro_scale_p_noise),
	_mage_p_noise(this, "EKF2_MAG_E_NOISE", false, &_params->mage_p_noise),
	_magb_p_noise(this, "EKF2_MAG_B_NOISE", false, &_params->magb_p_noise),
	_wind_vel_p_noise(this, "EKF2_WIND_NOISE", false, &_params->wind_vel_p_noise),
	_terrain_p_noise(this, "EKF2_TERR_NOISE", false, &_params->terrain_p_noise),
	_terrain_gradient(this, "EKF2_TERR_GRAD", false, &_params->terrain_gradient),
	_gps_vel_noise(this, "EKF2_GPS_V_NOISE", false, &_params->gps_vel_noise),
	_gps_pos_noise(this, "EKF2_GPS_P_NOISE", false, &_params->gps_pos_noise),
	_pos_noaid_noise(this, "EKF2_NOAID_NOISE", false, &_params->pos_noaid_noise),
	_baro_noise(this, "EKF2_BARO_NOISE", false, &_params->baro_noise),
	_baro_innov_gate(this, "EKF2_BARO_GATE", false, &_params->baro_innov_gate),
	_posNE_innov_gate(this, "EKF2_GPS_P_GATE", false, &_params->posNE_innov_gate),
	_vel_innov_gate(this, "EKF2_GPS_V_GATE", false, &_params->vel_innov_gate),
	_mag_heading_noise(this, "EKF2_HEAD_NOISE", false, &_params->mag_heading_noise),
	_mag_noise(this, "EKF2_MAG_NOISE", false, &_params->mag_noise),
	_eas_noise(this, "EKF2_EAS_NOISE", false, &_params->eas_noise),
	_mag_declination_deg(this, "EKF2_MAG_DECL", false, &_params->mag_declination_deg),
	_heading_innov_gate(this, "EKF2_HDG_GATE", false, &_params->heading_innov_gate),
	_mag_innov_gate(this, "EKF2_MAG_GATE", false, &_params->mag_innov_gate),
	_mag_decl_source(this, "EKF2_DECL_TYPE", false, &_params->mag_declination_source),
	_mag_fuse_type(this, "EKF2_MAG_TYPE", false, &_params->mag_fusion_type),
	_gps_check_mask(this, "EKF2_GPS_CHECK", false, &_params->gps_check_mask),
	_requiredEph(this, "EKF2_REQ_EPH", false, &_params->req_hacc),
	_requiredEpv(this, "EKF2_REQ_EPV", false, &_params->req_vacc),
	_requiredSacc(this, "EKF2_REQ_SACC", false, &_params->req_sacc),
	_requiredNsats(this, "EKF2_REQ_NSATS", false, &_params->req_nsats),
	_requiredGDoP(this, "EKF2_REQ_GDOP", false, &_params->req_gdop),
	_requiredHdrift(this, "EKF2_REQ_HDRIFT", false, &_params->req_hdrift),
	_requiredVdrift(this, "EKF2_REQ_VDRIFT", false, &_params->req_vdrift),
	_param_record_replay_msg(this, "EKF2_REC_RPL", false, &_publish_replay_mode),
	_fusion_mode(this, "EKF2_AID_MASK", false, &_params->fusion_mode),
	_vdist_sensor_type(this, "EKF2_HGT_MODE", false, &_params->vdist_sensor_type),
	_range_noise(this, "EKF2_RNG_NOISE", false, &_params->range_noise),
	_range_innov_gate(this, "EKF2_RNG_GATE", false, &_params->range_innov_gate),
	_rng_gnd_clearance(this, "EKF2_MIN_RNG", false, &_params->rng_gnd_clearance),
	_flow_noise(this, "EKF2_OF_N_MIN", false, &_params->flow_noise),
	_flow_noise_qual_min(this, "EKF2_OF_N_MAX", false, &_params->flow_noise_qual_min),
	_flow_qual_min(this, "EKF2_OF_QMIN", false, &_params->flow_qual_min),
	_flow_innov_gate(this, "EKF2_OF_GATE", false, &_params->flow_innov_gate),
	_flow_rate_max(this, "EKF2_OF_RMAX", false, &_params->flow_rate_max),
	_imu_pos_x(this, "EKF2_IMU_POS_X", false, &_params->imu_pos_body(0)),
	_imu_pos_y(this, "EKF2_IMU_POS_Y", false, &_params->imu_pos_body(1)),
	_imu_pos_z(this, "EKF2_IMU_POS_Z", false, &_params->imu_pos_body(2)),
	_gps_pos_x(this, "EKF2_GPS_POS_X", false, &_params->gps_pos_body(0)),
	_gps_pos_y(this, "EKF2_GPS_POS_Y", false, &_params->gps_pos_body(1)),
	_gps_pos_z(this, "EKF2_GPS_POS_Z", false, &_params->gps_pos_body(2)),
	_rng_pos_x(this, "EKF2_RNG_POS_X", false, &_params->rng_pos_body(0)),
	_rng_pos_y(this, "EKF2_RNG_POS_Y", false, &_params->rng_pos_body(1)),
	_rng_pos_z(this, "EKF2_RNG_POS_Z", false, &_params->rng_pos_body(2)),
	_flow_pos_x(this, "EKF2_OF_POS_X", false, &_params->flow_pos_body(0)),
	_flow_pos_y(this, "EKF2_OF_POS_Y", false, &_params->flow_pos_body(1)),
	_flow_pos_z(this, "EKF2_OF_POS_Z", false, &_params->flow_pos_body(2))
{

}

Ekf2::~Ekf2()
{

}

void Ekf2::print_status()
{
	warnx("local position OK %s", (_ekf.local_position_is_valid()) ? "[YES]" : "[NO]");
	warnx("global position OK %s", (_ekf.global_position_is_valid()) ? "[YES]" : "[NO]");
}

void Ekf2::task_main()
{
	// subscribe to relevant topics
	_sensors_sub = orb_subscribe(ORB_ID(sensor_combined));
	_gps_sub = orb_subscribe(ORB_ID(vehicle_gps_position));
	_airspeed_sub = orb_subscribe(ORB_ID(airspeed));
	_params_sub = orb_subscribe(ORB_ID(parameter_update));
	_optical_flow_sub = orb_subscribe(ORB_ID(optical_flow));
	_range_finder_sub = orb_subscribe(ORB_ID(distance_sensor));
	_actuator_armed_sub = orb_subscribe(ORB_ID(actuator_armed));
	_vehicle_land_detected_sub = orb_subscribe(ORB_ID(vehicle_land_detected));

	px4_pollfd_struct_t fds[2] = {};
	fds[0].fd = _sensors_sub;
	fds[0].events = POLLIN;
	fds[1].fd = _params_sub;
	fds[1].events = POLLIN;

	// initialise parameter cache
	updateParams();

	// initialize data structures outside of loop
	// because they will else not always be
	// properly populated
	sensor_combined_s sensors = {};
	vehicle_gps_position_s gps = {};
	airspeed_s airspeed = {};
	optical_flow_s optical_flow = {};
	distance_sensor_s range_finder = {};
	actuator_armed_s actuator_armed = {};

	while (!_task_should_exit) {
		int ret = px4_poll(fds, sizeof(fds) / sizeof(fds[0]), 1000);

		if (ret < 0) {
			// Poll error, sleep and try again
			usleep(10000);
			continue;

		} else if (ret == 0) {
			// Poll timeout or no new data, do nothing
			continue;
		}

		if (fds[1].revents & POLLIN) {
			// read from param to clear updated flag
			struct parameter_update_s update;
			orb_copy(ORB_ID(parameter_update), _params_sub, &update);
			updateParams();

			// fetch sensor data in next loop
			continue;

		} else if (!(fds[0].revents & POLLIN)) {
			// no new data
			continue;
		}

		bool gps_updated = false;
		bool airspeed_updated = false;
		bool optical_flow_updated = false;
		bool range_finder_updated = false;
		bool actuator_armed_updated = false;
		bool vehicle_land_detected_updated = false;

		orb_copy(ORB_ID(sensor_combined), _sensors_sub, &sensors);
		// update all other topics if they have new data
		orb_check(_gps_sub, &gps_updated);

		if (gps_updated) {
			orb_copy(ORB_ID(vehicle_gps_position), _gps_sub, &gps);
		}

		orb_check(_airspeed_sub, &airspeed_updated);

		if (airspeed_updated) {
			orb_copy(ORB_ID(airspeed), _airspeed_sub, &airspeed);
		}

		orb_check(_optical_flow_sub, &optical_flow_updated);

		if (optical_flow_updated) {
			orb_copy(ORB_ID(optical_flow), _optical_flow_sub, &optical_flow);
		}

		orb_check(_range_finder_sub, &range_finder_updated);

		if (range_finder_updated) {
			orb_copy(ORB_ID(distance_sensor), _range_finder_sub, &range_finder);
		}

		// in replay mode we are getting the actual timestamp from the sensor topic
		hrt_abstime now = 0;

		if (_replay_mode) {
			now = sensors.timestamp;

		} else {
			now = hrt_absolute_time();
		}

		// push imu data into estimator
		_ekf.setIMUData(now, sensors.gyro_integral_dt[0], sensors.accelerometer_integral_dt[0],
				&sensors.gyro_integral_rad[0], &sensors.accelerometer_integral_m_s[0]);

		// read mag data
		_ekf.setMagData(sensors.magnetometer_timestamp[0], &sensors.magnetometer_ga[0]);

		// read baro data
		_ekf.setBaroData(sensors.baro_timestamp[0], &sensors.baro_alt_meter[0]);

		// read gps data if available
		if (gps_updated) {
			struct gps_message gps_msg = {};
			gps_msg.time_usec = gps.timestamp_position;
			gps_msg.lat = gps.lat;
			gps_msg.lon = gps.lon;
			gps_msg.alt = gps.alt;
			gps_msg.fix_type = gps.fix_type;
			gps_msg.eph = gps.eph;
			gps_msg.epv = gps.epv;
			gps_msg.sacc = gps.s_variance_m_s;
			gps_msg.time_usec_vel = gps.timestamp_velocity;
			gps_msg.vel_m_s = gps.vel_m_s;
			gps_msg.vel_ned[0] = gps.vel_n_m_s;
			gps_msg.vel_ned[1] = gps.vel_e_m_s;
			gps_msg.vel_ned[2] = gps.vel_d_m_s;
			gps_msg.vel_ned_valid = gps.vel_ned_valid;
			gps_msg.nsats = gps.satellites_used;
			//TODO add gdop to gps topic
			gps_msg.gdop = 0.0f;

			_ekf.setGpsData(gps.timestamp_position, &gps_msg);
		}

		// read airspeed data if available
		float eas2tas = airspeed.true_airspeed_m_s / airspeed.indicated_airspeed_m_s;

		if (airspeed_updated && airspeed.true_airspeed_m_s > 7.0f) {
			_ekf.setAirspeedData(airspeed.timestamp, &airspeed.true_airspeed_m_s, &eas2tas);
		}

		if (optical_flow_updated) {
			flow_message flow;
			flow.flowdata(0) = optical_flow.pixel_flow_x_integral;
			flow.flowdata(1) = optical_flow.pixel_flow_y_integral;
			flow.quality = optical_flow.quality;
			flow.gyrodata(0) = optical_flow.gyro_x_rate_integral;
			flow.gyrodata(1) = optical_flow.gyro_y_rate_integral;
			flow.gyrodata(2) = optical_flow.gyro_z_rate_integral;
			flow.dt = optical_flow.integration_timespan;

			if (PX4_ISFINITE(optical_flow.pixel_flow_y_integral) &&
			    PX4_ISFINITE(optical_flow.pixel_flow_x_integral)) {
				_ekf.setOpticalFlowData(optical_flow.timestamp, &flow);
			}
		}

		if (range_finder_updated) {
			_ekf.setRangeData(range_finder.timestamp, &range_finder.current_distance);
		}

		orb_check(_actuator_armed_sub, &actuator_armed_updated);

		if (actuator_armed_updated) {
			orb_copy(ORB_ID(actuator_armed), _actuator_armed_sub, &actuator_armed);
			_ekf.set_arm_status(actuator_armed.armed);
		}

		orb_check(_vehicle_land_detected_sub, &vehicle_land_detected_updated);

		if (vehicle_land_detected_updated) {
			struct vehicle_land_detected_s vehicle_land_detected = {};
			orb_copy(ORB_ID(vehicle_land_detected), _vehicle_land_detected_sub, &vehicle_land_detected);
			_ekf.set_in_air_status(!vehicle_land_detected.landed);
		}

		// run the EKF update and output
		if (_ekf.update()) {
			// generate vehicle attitude quaternion data
			struct vehicle_attitude_s att = {};
			_ekf.copy_quaternion(att.q);
			matrix::Quaternion<float> q(att.q[0], att.q[1], att.q[2], att.q[3]);

			// generate control state data
			control_state_s ctrl_state = {};
			ctrl_state.timestamp = hrt_absolute_time();
			ctrl_state.roll_rate = _lp_roll_rate.apply(sensors.gyro_rad_s[0]);
			ctrl_state.pitch_rate = _lp_pitch_rate.apply(sensors.gyro_rad_s[1]);
			ctrl_state.yaw_rate = _lp_yaw_rate.apply(sensors.gyro_rad_s[2]);

			// Velocity in body frame
			float velocity[3];
			_ekf.get_velocity(velocity);
			Vector3f v_n(velocity);
			matrix::Dcm<float> R_to_body(q.inversed());
			Vector3f v_b = R_to_body * v_n;
			ctrl_state.x_vel = v_b(0);
			ctrl_state.y_vel = v_b(1);
			ctrl_state.z_vel = v_b(2);


			// Local Position NED
			float position[3];
			_ekf.get_position(position);
			ctrl_state.x_pos = position[0];
			ctrl_state.y_pos = position[1];
			ctrl_state.z_pos = position[2];

			// Attitude quaternion
			ctrl_state.q[0] = q(0);
			ctrl_state.q[1] = q(1);
			ctrl_state.q[2] = q(2);
			ctrl_state.q[3] = q(3);

			// Acceleration data
			matrix::Vector<float, 3> acceleration = {&sensors.accelerometer_m_s2[0]};

			float accel_bias = 0.0f;
			_ekf.get_accel_bias(&accel_bias);
			ctrl_state.x_acc = acceleration(0);
			ctrl_state.y_acc = acceleration(1);
			ctrl_state.z_acc = acceleration(2) - accel_bias;

			// compute lowpass filtered horizontal acceleration
			acceleration = R_to_body.transpose() * acceleration;
			_acc_hor_filt = 0.95f * _acc_hor_filt + 0.05f * sqrtf(acceleration(0) * acceleration(0) + acceleration(
						1) * acceleration(1));
			ctrl_state.horz_acc_mag = _acc_hor_filt;

			// Airspeed - take airspeed measurement directly here as no wind is estimated
			if (PX4_ISFINITE(airspeed.indicated_airspeed_m_s) && hrt_absolute_time() - airspeed.timestamp < 1e6
			    && airspeed.timestamp > 0) {
				ctrl_state.airspeed = airspeed.indicated_airspeed_m_s;
				ctrl_state.airspeed_valid = true;

			} else {
				ctrl_state.airspeed_valid = false;
			}

			// publish control state data
			if (_control_state_pub == nullptr) {
				_control_state_pub = orb_advertise(ORB_ID(control_state), &ctrl_state);

			} else {
				orb_publish(ORB_ID(control_state), _control_state_pub, &ctrl_state);
			}


			// generate remaining vehicle attitude data
			att.timestamp = hrt_absolute_time();
			matrix::Euler<float> euler(q);
			att.roll = euler(0);
			att.pitch = euler(1);
			att.yaw = euler(2);

			att.q[0] = q(0);
			att.q[1] = q(1);
			att.q[2] = q(2);
			att.q[3] = q(3);
			att.q_valid = true;

			att.rollspeed = sensors.gyro_rad_s[0];
			att.pitchspeed = sensors.gyro_rad_s[1];
			att.yawspeed = sensors.gyro_rad_s[2];

			// publish vehicle attitude data
			if (_att_pub == nullptr) {
				_att_pub = orb_advertise(ORB_ID(vehicle_attitude), &att);

			} else {
				orb_publish(ORB_ID(vehicle_attitude), _att_pub, &att);
			}

			// generate vehicle local position data
			struct vehicle_local_position_s lpos = {};
			float pos[3] = {};
			float vel[3] = {};

			lpos.timestamp = hrt_absolute_time();

			// Position of body origin in local NED frame
			_ekf.get_position(pos);
			lpos.x = pos[0];
			lpos.y = pos[1];
			lpos.z = pos[2];

			// Velocity of body origin in local NED frame (m/s)
			_ekf.get_velocity(vel);
			lpos.vx = vel[0];
			lpos.vy = vel[1];
			lpos.vz = vel[2];

			// TODO: better status reporting
			lpos.xy_valid = _ekf.local_position_is_valid();
			lpos.z_valid = true;
			lpos.v_xy_valid = _ekf.local_position_is_valid();
			lpos.v_z_valid = true;

			// Position of local NED origin in GPS / WGS84 frame
			struct map_projection_reference_s ekf_origin = {};
			// true if position (x, y) is valid and has valid global reference (ref_lat, ref_lon)
			_ekf.get_ekf_origin(&lpos.ref_timestamp, &ekf_origin, &lpos.ref_alt);
			lpos.xy_global = _ekf.global_position_is_valid();
			lpos.z_global = true;                                // true if z is valid and has valid global reference (ref_alt)
			lpos.ref_lat = ekf_origin.lat_rad * 180.0 / M_PI; // Reference point latitude in degrees
			lpos.ref_lon = ekf_origin.lon_rad * 180.0 / M_PI; // Reference point longitude in degrees

			// The rotation of the tangent plane vs. geographical north
			lpos.yaw = att.yaw;

			float terrain_vpos;
			lpos.dist_bottom_valid = _ekf.get_terrain_vert_pos(&terrain_vpos);
			lpos.dist_bottom = terrain_vpos - pos[2]; // Distance to bottom surface (ground) in meters
			lpos.dist_bottom_rate = -vel[2]; // Distance to bottom surface (ground) change rate
			lpos.surface_bottom_timestamp	= hrt_absolute_time(); // Time when new bottom surface found

			// TODO: uORB definition does not define what these variables are. We have assumed them to be horizontal and vertical 1-std dev accuracy in metres
			Vector3f pos_var, vel_var;
			_ekf.get_pos_var(pos_var);
			_ekf.get_vel_var(vel_var);
			lpos.eph = sqrt(pos_var(0) + pos_var(1));
			lpos.epv = sqrt(pos_var(2));

			// publish vehicle local position data
			if (_lpos_pub == nullptr) {
				_lpos_pub = orb_advertise(ORB_ID(vehicle_local_position), &lpos);

			} else {
				orb_publish(ORB_ID(vehicle_local_position), _lpos_pub, &lpos);
			}

			// generate and publish global position data
			struct vehicle_global_position_s global_pos = {};

			if (_ekf.global_position_is_valid()) {
				global_pos.timestamp = hrt_absolute_time(); // Time of this estimate, in microseconds since system start
				global_pos.time_utc_usec = gps.time_utc_usec; // GPS UTC timestamp in microseconds

				double est_lat, est_lon;
				map_projection_reproject(&ekf_origin, lpos.x, lpos.y, &est_lat, &est_lon);
				global_pos.lat = est_lat; // Latitude in degrees
				global_pos.lon = est_lon; // Longitude in degrees

				global_pos.alt = -pos[2] + lpos.ref_alt; // Altitude AMSL in meters

				global_pos.vel_n = vel[0]; // Ground north velocity, m/s
				global_pos.vel_e = vel[1]; // Ground east velocity, m/s
				global_pos.vel_d = vel[2]; // Ground downside velocity, m/s

				global_pos.yaw = euler(2); // Yaw in radians -PI..+PI.

				global_pos.eph = sqrt(pos_var(0) + pos_var(1));; // Standard deviation of position estimate horizontally
				global_pos.epv = sqrt(pos_var(2)); // Standard deviation of position vertically

				// TODO: implement terrain estimator
				global_pos.terrain_alt = 0.0f; // Terrain altitude in m, WGS84
				global_pos.terrain_alt_valid = false; // Terrain altitude estimate is valid
				// TODO use innovatun consistency check timouts to set this
				global_pos.dead_reckoning = false; // True if this position is estimated through dead-reckoning

				global_pos.pressure_alt = sensors.baro_alt_meter[0]; // Pressure altitude AMSL (m)

				if (_vehicle_global_position_pub == nullptr) {
					_vehicle_global_position_pub = orb_advertise(ORB_ID(vehicle_global_position), &global_pos);

				} else {
					orb_publish(ORB_ID(vehicle_global_position), _vehicle_global_position_pub, &global_pos);
				}
			}

		} else if (_replay_mode) {
			// in replay mode we have to tell the replay module not to wait for an update
			// we do this by publishing an attitude with zero timestamp
			struct vehicle_attitude_s att = {};
			att.timestamp = 0;

			if (_att_pub == nullptr) {
				_att_pub = orb_advertise(ORB_ID(vehicle_attitude), &att);

			} else {
				orb_publish(ORB_ID(vehicle_attitude), _att_pub, &att);
			}
		}

		// publish estimator status
		struct estimator_status_s status = {};
		status.timestamp = hrt_absolute_time();
		_ekf.get_state_delayed(status.states);
		_ekf.get_covariances(status.covariances);
		_ekf.get_gps_check_status(&status.gps_check_fail_flags);
		_ekf.get_control_mode(&status.control_mode_flags);

		if (_estimator_status_pub == nullptr) {
			_estimator_status_pub = orb_advertise(ORB_ID(estimator_status), &status);

		} else {
			orb_publish(ORB_ID(estimator_status), _estimator_status_pub, &status);
		}

		// Publish wind estimate
		struct wind_estimate_s wind_estimate = {};
		wind_estimate.timestamp = hrt_absolute_time();
		wind_estimate.windspeed_north = status.states[22];
		wind_estimate.windspeed_east = status.states[23];
		wind_estimate.covariance_north = status.covariances[22];
		wind_estimate.covariance_east = status.covariances[23];

		if (_wind_pub == nullptr) {
			_wind_pub = orb_advertise(ORB_ID(wind_estimate), &wind_estimate);

		} else {
			orb_publish(ORB_ID(wind_estimate), _wind_pub, &wind_estimate);
		}

		// publish estimator innovation data
		struct ekf2_innovations_s innovations = {};
		innovations.timestamp = hrt_absolute_time();
		_ekf.get_vel_pos_innov(&innovations.vel_pos_innov[0]);
		_ekf.get_mag_innov(&innovations.mag_innov[0]);
		_ekf.get_heading_innov(&innovations.heading_innov);
		_ekf.get_airspeed_innov(&innovations.airspeed_innov);
		_ekf.get_flow_innov(&innovations.flow_innov[0]);
		_ekf.get_hagl_innov(&innovations.hagl_innov);

		_ekf.get_vel_pos_innov_var(&innovations.vel_pos_innov_var[0]);
		_ekf.get_mag_innov_var(&innovations.mag_innov_var[0]);
		_ekf.get_heading_innov_var(&innovations.heading_innov_var);
		_ekf.get_airspeed_innov_var(&innovations.airspeed_innov_var);
		_ekf.get_flow_innov_var(&innovations.flow_innov_var[0]);
		_ekf.get_hagl_innov_var(&innovations.hagl_innov_var);

		if (_estimator_innovations_pub == nullptr) {
			_estimator_innovations_pub = orb_advertise(ORB_ID(ekf2_innovations), &innovations);

		} else {
			orb_publish(ORB_ID(ekf2_innovations), _estimator_innovations_pub, &innovations);
		}

		// save the declination to the EKF2_MAG_DECL parameter when a dis-arm event is detected
		if ((_params->mag_declination_source & (1 << 1)) && _prev_motors_armed && !actuator_armed.armed) {
			float decl_deg;
			_ekf.copy_mag_decl_deg(&decl_deg);
			_mag_declination_deg.set(decl_deg);
		}

		// publish replay message if in replay mode
		bool publish_replay_message = (bool)_param_record_replay_msg.get();

		if (publish_replay_message) {
			struct ekf2_replay_s replay = {};
			replay.time_ref = now;
			replay.gyro_integral_dt = sensors.gyro_integral_dt[0];
			replay.accelerometer_integral_dt = sensors.accelerometer_integral_dt[0];
			replay.magnetometer_timestamp = sensors.magnetometer_timestamp[0];
			replay.baro_timestamp = sensors.baro_timestamp[0];
			memcpy(&replay.gyro_integral_rad[0], &sensors.gyro_integral_rad[0], sizeof(replay.gyro_integral_rad));
			memcpy(&replay.accelerometer_integral_m_s[0], &sensors.accelerometer_integral_m_s[0],
			       sizeof(replay.accelerometer_integral_m_s));
			memcpy(&replay.magnetometer_ga[0], &sensors.magnetometer_ga[0], sizeof(replay.magnetometer_ga));
			replay.baro_alt_meter = sensors.baro_alt_meter[0];

			// only write gps data if we had a gps update.
			if (gps_updated) {
				replay.time_usec = gps.timestamp_position;
				replay.time_usec_vel = gps.timestamp_velocity;
				replay.lat = gps.lat;
				replay.lon = gps.lon;
				replay.alt = gps.alt;
				replay.fix_type = gps.fix_type;
				replay.nsats = gps.satellites_used;
				replay.eph = gps.eph;
				replay.epv = gps.epv;
				replay.sacc = gps.s_variance_m_s;
				replay.vel_m_s = gps.vel_m_s;
				replay.vel_n_m_s = gps.vel_n_m_s;
				replay.vel_e_m_s = gps.vel_e_m_s;
				replay.vel_d_m_s = gps.vel_d_m_s;
				replay.vel_ned_valid = gps.vel_ned_valid;

			} else {
				// this will tell the logging app not to bother logging any gps replay data
				replay.time_usec = 0;
			}

			if (optical_flow_updated) {
				replay.flow_timestamp = optical_flow.timestamp;
				replay.flow_pixel_integral[0] = optical_flow.pixel_flow_x_integral;
				replay.flow_pixel_integral[1] = optical_flow.pixel_flow_y_integral;
				replay.flow_gyro_integral[0] = optical_flow.gyro_x_rate_integral;
				replay.flow_gyro_integral[1] = optical_flow.gyro_y_rate_integral;
				replay.flow_time_integral = optical_flow.integration_timespan;
				replay.flow_quality = optical_flow.quality;

			} else {
				replay.flow_timestamp = 0;
			}

			if (range_finder_updated) {
				replay.rng_timestamp = range_finder.timestamp;
				replay.range_to_ground = range_finder.current_distance;

			} else {
				replay.rng_timestamp = 0;
			}

			if (_replay_pub == nullptr) {
				_replay_pub = orb_advertise(ORB_ID(ekf2_replay), &replay);

			} else {
				orb_publish(ORB_ID(ekf2_replay), _replay_pub, &replay);
			}
		}
	}

	delete ekf2::instance;
	ekf2::instance = nullptr;
}

void Ekf2::task_main_trampoline(int argc, char *argv[])
{
	ekf2::instance->task_main();
}

int Ekf2::start()
{
	ASSERT(_control_task == -1);

	/* start the task */
	_control_task = px4_task_spawn_cmd("ekf2",
					   SCHED_DEFAULT,
					   SCHED_PRIORITY_MAX - 5,
					   9000,
					   (px4_main_t)&Ekf2::task_main_trampoline,
					   nullptr);

	if (_control_task < 0) {
		PX4_WARN("task start failed");
		return -errno;
	}

	return OK;
}

int ekf2_main(int argc, char *argv[])
{
	if (argc < 2) {
		PX4_WARN("usage: ekf2 {start|stop|status}");
		return 1;
	}

	if (!strcmp(argv[1], "start")) {

		if (ekf2::instance != nullptr) {
			PX4_WARN("already running");
			return 1;
		}

		ekf2::instance = new Ekf2();

		if (ekf2::instance == nullptr) {
			PX4_WARN("alloc failed");
			return 1;
		}

		if (argc >= 3) {
			if (!strcmp(argv[2], "--replay")) {
				ekf2::instance->set_replay_mode(true);
			}
		}

		if (OK != ekf2::instance->start()) {
			delete ekf2::instance;
			ekf2::instance = nullptr;
			PX4_WARN("start failed");
			return 1;
		}

		return 0;
	}

	if (!strcmp(argv[1], "stop")) {
		if (ekf2::instance == nullptr) {
			PX4_WARN("not running");
			return 1;
		}

		ekf2::instance->exit();

		// wait for the destruction of the instance
		while (ekf2::instance != nullptr) {
			usleep(50000);
		}

		return 0;
	}

	if (!strcmp(argv[1], "print")) {
		if (ekf2::instance != nullptr) {

			return 0;
		}

		return 1;
	}

	if (!strcmp(argv[1], "status")) {
		if (ekf2::instance) {
			PX4_WARN("running");
			ekf2::instance->print_status();
			return 0;

		} else {
			PX4_WARN("not running");
			return 1;
		}
	}

	PX4_WARN("unrecognized command");
	return 1;
}
