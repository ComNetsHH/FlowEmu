#!/usr/bin/env python3

# FlowEmu - Flow-Based Network Emulator
# Copyright (c) 2021 Institute of Communication Networks (ComNets),
#                    Hamburg University of Technology (TUHH),
#                    https://www.tuhh.de/comnets
# Copyright (c) 2021 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import json
import os
import re
import subprocess
import sys
import time
import toml
from copy import deepcopy
from datetime import datetime
from threading import Thread

start_time = time.time()


class Environment:
	path = ""
	metadata = {}
	operating_modes = {}
	config = {}

	def __init__(self, path):
		self.path = path

		# Load config file
		try:
			with open(self.path + "/config.toml", "r") as config_file:
				environment_toml = toml.load(config_file)
		except FileNotFoundError as error:
			raise RuntimeError("Cannot open config file '" + self.path + "/config.toml': " + str(error))
		except toml.decoder.TomlDecodeError as error:
			raise RuntimeError("Invalid environment config file '" + self.path + "/config.toml': " + str(error))

		# Get metadata
		self.metadata["name"] = str(environment_toml["metadata"]["name"])
		self.metadata["description"] = str(environment_toml["metadata"]["description"])

		# Get supported operating modes
		self.operating_modes["testcase"] = environment_toml["operating_modes"]["testcase"] if "operating_modes" in environment_toml and "testcase" in environment_toml["operating_modes"] else True
		self.operating_modes["interactive"] = environment_toml["operating_modes"]["interactive"] if "operating_modes" in environment_toml and "interactive" in environment_toml["operating_modes"] else True

		# Get config
		self.config = environment_toml["config"]

	# Run build script
	def build(self):
		print("\033[1;33mBuild\033[0m")
		status = os.system(self.path + "/build.sh")
		if os.WEXITSTATUS(status) != 0:
			raise RuntimeError

	# Run setup script
	def setup(self):
		print("\033[1;33mSetup\033[0m")
		status = os.system(self.path + "/setup.sh")
		if os.WEXITSTATUS(status) != 0:
			raise RuntimeError

	# Run cleanup script
	def cleanup(self):
		print("\033[1;33mCleanup\033[0m")
		status = os.system(self.path + "/cleanup.sh")
		if os.WEXITSTATUS(status) != 0:
			raise RuntimeError


class Config:
	metadata = {}
	testcases = []

	def __init__(self, path):
		# Load test cases file
		try:
			with open(path, "r") as testcases_file:
				testcases_toml = toml.load(testcases_file)
		except FileNotFoundError as error:
			raise RuntimeError("Cannot open test cases file '" + path + "': " + str(error))
		except toml.decoder.TomlDecodeError as error:
			raise RuntimeError("Invalid test cases file '" + path + "': " + str(error))

		# Get metadata
		self.metadata["name"] = str(testcases_toml["metadata"]["name"])
		self.metadata["description"] = str(testcases_toml["metadata"]["description"])
		self.metadata["author"] = str(testcases_toml["metadata"]["author"])

		# Iterate over test cases
		for name, config in testcases_toml.items():
			# Skip metadata
			if name == "metadata":
				continue

			# Add general parameters to test case
			testcase = deepcopy(testcases_toml["general"])
			for key, value in config.items():
				if key in testcase and isinstance(testcase[key], dict) and isinstance(value, dict):
					testcase[key].update(value)
				else:
					testcase[key] = value

			# Set test case name
			testcase["name"] = name

			# Add test case to test cases list
			self.testcases.append(testcase)


class Results:
	path = ""

	def __init__(self, path, name):
		# Create results directory
		self.path = path + "/" + name + " - " + datetime.fromtimestamp(start_time).strftime("%Y-%m-%d %H-%M-%S")
		os.system("mkdir -p \"" + self.path + "\"")

	# Write metadata file
	def writeMetadata(self, environment_metadata, testcases_metadata):
		metadata = {}
		metadata["environment"] = environment_metadata
		metadata["testcases"] = testcases_metadata
		metadata["timestamp"] = datetime.fromtimestamp(start_time).strftime("%Y-%m-%d %H-%M-%S")
		metadata["git_head"] = subprocess.check_output("git rev-parse HEAD", shell=True).decode("utf-8", errors="ignore").strip()
		metadata["git_dirty_src"] = (subprocess.check_output("git diff --shortstat src/", shell=True).decode("utf-8", errors="ignore").strip() != "")

		with open(self.path + "/metadata.json", "w") as metadata_file:
			json.dump(metadata, metadata_file)

	# Write test case file
	def writeTestcase(self, testcase):
		with open(self.path + "/testcase_" + testcase["name"] + ".json", "w") as testcase_file:
			json.dump(testcase, testcase_file)

	# Get results directory path
	def getResultsDirectoryPath(self):
		return self.path

class Process:
	cmd = ""
	process = None
	thread_log = None
	thread_error = None
	logfile = None
	verbose = True

	def __init__(self, name, docker_container = None, color = 0):
		self.name = name
		self.docker_container = docker_container
		self.color = color

	# Open log file
	def setLogfile(self, path):
		self.logfile = open(path, "w")

	# Set if process output should be printed to the command-line
	def setVerbose(self, verbose):
		self.verbose = verbose

	# Handle output stream
	def outputstream(self, stream, level):
		while True:
			# Get line
			line = stream.readline()
			if line:
				# Get current timestamp
				timestamp = str(round((time.time() - start_time)*1000, 1))

				# Write line to log file
				if self.logfile:
					self.logfile.write("[" + timestamp + "] " + line.rstrip().decode("utf-8", errors="replace") + "\n")

				# Write line to command-line
				if level == "error":
					print("\033[1;31m[" + timestamp + " - " + self.name + "] " + line.rstrip().decode("utf-8", errors="replace") + "\033[0m\r")
				elif self.verbose:
					print("\033[1;" + str(int(self.color)) + "m[" + timestamp + " - " + self.name + "] " + line.rstrip().decode("utf-8", errors="replace") + "\033[0m\r")
			else:
				break

	# Run command
	def run(self, cmd):
		if not self.process:
			self.cmd = cmd
			print(self.name + " command-line call: " + self.cmd)

			if self.docker_container != None:
				# Run command in docker container
				self.process = subprocess.Popen("docker exec " + self.docker_container + " " + self.cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, preexec_fn=os.setpgrp)
			else:
				# Run command on host
				self.process = subprocess.Popen("exec " + self.cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True, preexec_fn=os.setpgrp)

			# Create output stream threads
			self.thread_log = Thread(target=self.outputstream, args=(self.process.stdout, "log"))
			self.thread_log.start()
			self.thread_error = Thread(target=self.outputstream, args=(self.process.stderr, "error"))
			self.thread_error.start()

	# Wait for process to finish, join output stream threads and close log file
	def wait(self):
		# Wait for process to finish
		if self.process:
			return_code = self.process.wait()
			self.process = None

			print(self.name + " return code: " + str(return_code))

		# Join standard output stream thread
		if self.thread_log:
			self.thread_log.join()
			self.thread_log = None

		# Join error output stream thread
		if self.thread_error:
			self.thread_error.join()
			self.thread_error = None

		# Close log file
		if self.logfile:
			self.logfile.close()
			self.logfile = None

	# Send stop signal to process and wait
	def stop(self):
		if self.process:
			print("Stop " + self.name + "!")
			if self.docker_container != None:
				os.system("docker top " + self.docker_container + " | grep \"" + self.cmd + "\" | awk '{print $2}' | xargs sudo kill --signal SIGTERM")
			else:
				self.process.terminate()

		self.wait()


class SudoLoop:
	thread = None

	# Reset sudo timeout or ask for sudo password
	def validate(self):
		status = os.system("sudo -v")
		if os.WEXITSTATUS(status) != 0:
			raise RuntimeError("Sudo password is required to continue!")

	# Sudo timeout reset loop
	def loop(self):
		while True:
			time.sleep(60)
			self.validate()

	# Ask for sudo password and start sudo timeout reset loop
	def run(self):
		self.validate()

		self.thread = Thread(target=self.loop, daemon=True)
		self.thread.start()


# Get value from nested dictionary if the key exists, otherwise return the default value
def get(dict, keys, default=None):
	element = dict

	for key in keys:
		if key in element:
			element = element[key]
		else:
			return default

	return element


# Main
def main():
	# Check command-line arguments for environment config directory
	if(len(sys.argv) < 2):
		print("Please provide an environment config directory as command-line argument!")
		exit(1)

	# Load environment config
	try:
		environment = Environment(str(sys.argv[1]))
	except RuntimeError as error:
		print(error)
		exit(1)

	# Check command-line arguments for test cases file to get operating mode
	operating_mode = "testcase" if len(sys.argv) >= 3 else "interactive"

	# Check for incompatible operating mode
	if operating_mode == "testcase" and environment.operating_modes["testcase"] == False:
		print("Test case mode not supported by environment!")
		exit(1)
	if operating_mode == "interactive" and environment.operating_modes["interactive"] == False:
		print("Interactive mode not supported by environment!")
		exit(1)

	# Load test cases file
	if operating_mode == "testcase":
		try:
			config = Config(str(sys.argv[2]))
		except RuntimeError as error:
			print(error)
			exit(1)

	# Ask for sudo password and start sudo timeout reset loop
	sudo_loop = SudoLoop()
	try:
		sudo_loop.run()
	except RuntimeError:
		exit(1)

	# Build FlowEmu
	try:
		environment.build()
	except RuntimeError:
		print("Error while building FlowEmu!")
		exit(1)

	# Prepare environment
	try:
		environment.cleanup()
	except RuntimeError:
		print("Error while cleaning up environment!")
		exit(1)

	try:
		environment.setup()
	except RuntimeError:
		print("Error while setting up environment!")
		exit(1)

	# Run FlowEmu in test case mode
	if operating_mode == "testcase":
		# Print config
		#print(config.metadata)
		#print(config.testcases)

		# Create results directory and metadata file
		results = Results("results", config.metadata["name"])
		results.writeMetadata(environment.metadata, config.metadata)

		# Run test cases
		print("\033[1;33mRun test cases \'" + config.metadata["name"] + "\' by " + config.metadata["author"] + "\033[0m")
		try:
			# Run test case
			for testcase in config.testcases:
				# Skip other test cases if a specific test case is given as command-line argument
				if len(sys.argv) > 3 and sys.argv[3] != testcase["name"]:
					print("\033[1;33m--> Skip test case \'" + testcase["name"] + "\'\033[0m")
					continue

				# Write test case to results directory
				results.writeTestcase(testcase)

				# Repeat test case
				for repetition in range(0, testcase["repetitions"]):
					testcase["repetition"] = repetition
					results_path = results.getResultsDirectoryPath() + "/" + testcase["name"] + "_rep" + str(testcase["repetition"])
					print("\033[1;33m--> Run test case \'" + testcase["name"] + "\' - repetition " + str(testcase["repetition"]) + "\033[0m")

					# Get graph file from test case
					graph_file = ""
					if "graph-file" in testcase:
						graph_file = " --graph-file=" + testcase["graph-file"]

					# Get module parameters from test case
					module_parameters = ""
					for module in testcase.keys():
						if(isinstance(testcase[module], dict)):
							for parameter, value in testcase[module].items():
								# Evaluate Python expression
								if isinstance(value, str):
									value = eval(value)

								# Convert boolean to integer
								if isinstance(value, bool):
									value = int(value)

								module_parameters += " --" + module + "." + parameter + "=" + re.escape(str(value))

					# Setup processes
					process_logger = Process("Logger", color=37)
					process_control = Process("Control", color=37)
					process_source = Process("Source", docker_container=get(environment.config, ("docker_container", "source")), color=34)
					process_channel = Process("Channel", docker_container=get(environment.config, ("docker_container", "channel")), color=35)
					process_sink = Process("Sink", docker_container=get(environment.config, ("docker_container", "sink")), color=36)

					# Start processes
					process_logger.setVerbose(False)
					process_logger.setLogfile(results_path + ".log")
					process_logger.run("mosquitto_sub -h " + get(environment.config, ("mqtt", "host"), "") + " -t '#' -v")

					process_channel.setLogfile(results_path + "_channel.out")
					process_channel.run(get(environment.config, ("run_prefix", "channel"), "") + " " + "flowemu --mqtt-host=" + get(environment.config, ("mqtt", "host"), "") + " --interface-source=" + get(environment.config, ("interface", "source"), "") + " --interface-sink=" + get(environment.config, ("interface", "sink"), "") + graph_file + module_parameters)

					if "sink-command" in testcase and testcase["sink-command"] != "":
						process_sink.setLogfile(results_path + "_sink.out")
						process_sink.run(get(environment.config, ("run_prefix", "sink"), "") + " " + testcase["sink-command"])

					time.sleep(1)

					if "control-command" in testcase and testcase["control-command"] != "":
						process_control.setLogfile(results_path + "_control.out")
						process_control.run(get(environment.config, ("run_prefix", "control"), "") + " " + testcase["control-command"])

					if "source-command" in testcase and testcase["source-command"] != "":
						process_source.setLogfile(results_path + "_source.out")
						process_source.run(get(environment.config, ("run_prefix", "source"), "") + " " + testcase["source-command"])

					# Wait for source process to finish before stopping all other processes
					if "control-command" in testcase and testcase["control-command"] != "":
						process_control.wait()
						process_source.stop()
					else:
						process_source.wait()
						process_control.stop()
					process_sink.stop()
					process_channel.stop()
					process_logger.stop()

		# Catch keyboard interrupts
		except KeyboardInterrupt:
			# Cleanly terminate all processes
			process_control.stop()
			process_source.stop()
			process_sink.stop()
			process_channel.stop()
			process_logger.stop()

	# Run FlowEmu in interactive mode
	if operating_mode == "interactive":
		try:
			print("\033[1;33mRun FlowEmu in interactive mode\033[0m")

			process_channel = Process("Channel", docker_container=get(environment.config, ("docker_container", "channel")), color=35)
			process_channel.run(get(environment.config, ("run_prefix", "channel"), "") + " " + "flowemu --mqtt-host=" + get(environment.config, ("mqtt", "host"), "") + " --interface-source=" + get(environment.config, ("interface", "source"), "") + " --interface-sink=" + get(environment.config, ("interface", "sink"), ""))
			process_channel.wait()

		# Catch keyboard interrupts
		except KeyboardInterrupt:
			process_channel.stop()

	# Cleanup environment
	try:
		environment.cleanup()
	except RuntimeError:
		print("Error while cleaning up environment!")
		exit(1)

	exit(0)

if __name__ == "__main__":
	main()
