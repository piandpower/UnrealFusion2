/*  This file is part of UnrealFusion, a sensor fusion plugin for VR in the Unreal Engine
    Copyright (C) 2017 Jake Fountain
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "UnrealFusion.h"
#include "Core.h"
#include <chrono>

namespace fusion {

	void Core::addBoneNode(const NodeDescriptor & node, const NodeDescriptor & parent, const Transform3D& boneTransform)
	{
		skeleton.addNode(node, parent);
		skeleton.setBoneForNode(node, boneTransform);
	}

	void Core::addPoseNode(const NodeDescriptor & node, const NodeDescriptor & parent, const Transform3D& poseInitial)
	{
		skeleton.addNode(node, parent);
		skeleton.setPoseNode(node, poseInitial);
	}

	void Core::finaliseSetup()
	{
		skeleton.enumerateHeirarchy();
	}

	//Adds a new measurement to the system
	void Core::addMeasurement(const Measurement::Ptr& m, const NodeDescriptor& node) {
		m->getSensor()->addNode(node);
		measurement_buffer.push_back(m);
	}

	//Adds a new measurement to the system
	void Core::addMeasurement(const Measurement::Ptr& m, const std::vector<NodeDescriptor>& nodes) {
		//Add nodes which the measurement might correspond to - actually gets stored in the sensor pointer
		for(auto& n : nodes){
			m->getSensor()->addNode(n);
		}
		measurement_buffer.push_back(m);
	}

	//Computes data added since last fuse() call. Should be called repeatedly	
	void Core::fuse() {
		//TODO: add ifdefs for profiling
		//Add new data to calibration, with checking for usefulness
		//utility::profiler.startTimer("Correlator");
		correlator.addMeasurementGroup(measurement_buffer);
		correlator.identify();
		//utility::profiler.endTimer("Correlator");
		if(correlator.isStable() || true){
			//utility::profiler.startTimer("Calibrator add");
			calibrator.addMeasurementGroup(measurement_buffer);
			//utility::profiler.endTimer("Calibrator add");
			//utility::profiler.startTimer("Calibrate");
			calibrator.calibrate();
			//utility::profiler.endTimer("Calibrate");
			if(calibrator.isStable() || true){
				//utility::profiler.startTimer("Fuse");
				skeleton.addMeasurementGroup(measurement_buffer);
				skeleton.fuse();
				//utility::profiler.endTimer("Fuse");
			}
		}	
		//utility::profiler.startTimer("ClearMeasurements");
		measurement_buffer.clear();
		//utility::profiler.endTimer("ClearMeasurements");
		//TODO: do this less often
		//FUSION_LOG(utility::profiler.getReport());
	}

	CalibrationResult Core::getCalibrationResult(SystemDescriptor s1, SystemDescriptor s2) {
		return calibrator.getResultsFor(s1, s2);
	}

	Transform3D Core::getNodeGlobalPose(const NodeDescriptor& node){
		return skeleton.getNodeGlobalPose(node);
	}

	Transform3D Core::getNodeLocalPose(const NodeDescriptor & node)
	{
		return skeleton.getNodeLocalPose(node);
	}

	NodeDescriptor Core::getCorrelationResult(SystemDescriptor system, SensorID id) {
		if (sensors.count(system) > 0 &&
			sensors[system].count(id) > 0)
		{
			return sensors[system][id]->getNode();
		}
		else {
			return "UNKNOWN";
		}
	}
	//Called by owner of the Core object
	void Core::setMeasurementSensorInfo(Measurement::Ptr & m, SystemDescriptor system, SensorID id)
	{
		//If we haven't seen this sensor already, initialise
		if (utility::safeAccess(sensors, system).count(id) == 0) {
			utility::safeAccess(sensors, system)[id] = std::make_unique<Sensor>();
			utility::safeAccess(sensors, system)[id]->system = system;
			utility::safeAccess(sensors, system)[id]->id = id;
		}
		//Set pointer in measurement
		m->setSensor(sensors[system][id]);
	}

}
