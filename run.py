#!/usr/bin/env python

import json
import os
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
	config = {}

	def __init__(self, path):
		self.path = path

		# Load config file
		with open(self.path + "/config.toml", "r") as config_file:
			environment_toml = toml.load(config_file)

		# Get metadata
		self.metadata["name"] = str(environment_toml["metadata"]["name"])
		self.metadata["description"] = str(environment_toml["metadata"]["description"])

		# Get config
		self.config = environment_toml["config"]

	# Run build script
	def build(self):
		print("\033[1;33mBuild\033[0m")
		os.system(self.path + "/build.sh")

	# Run setup script
	def setup(self):
		print("\033[1;33mSetup\033[0m")
		os.system(self.path + "/setup.sh")

	# Run cleanup script
	def cleanup(self):
		print("\033[1;33mCleanup\033[0m")
		os.system(self.path + "/cleanup.sh")


class Config:
	metadata = {}
	testcases = []

	def __init__(self, path):
		# Load test cases file
		with open(path, "r") as testcases_file:
			testcases_toml = toml.load(testcases_file)

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
			testcase.update(config)

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
				if self.verbose:
					if level == "error":
						print("\033[1;31m[" + timestamp + " - " + self.name + "] " + line.rstrip().decode("utf-8", errors="replace") + "\033[0m\r")
					else:
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
		print("Please provide an environment config directory!")
		exit(1)

	# Load environment config
	environment = Environment(str(sys.argv[1]))

	# Build emulator
	environment.build()

	# Check command-line arguments for test cases file
	if(len(sys.argv) < 3):
		print("Please provide a test cases file!")
		exit(1)

	# Load test cases file
	config = Config(str(sys.argv[2]))

	# Print config
	#print(config.metadata)
	#print(config.testcases)

	# Create results directory and metadata file
	results = Results("results", config.metadata["name"])
	results.writeMetadata(environment.metadata, config.metadata)

	# Prepare environment
	environment.cleanup()
	environment.setup()

	# Run test cases
	print("\033[1;33m--> Run test cases \'" + config.metadata["name"] + "\' by " + config.metadata["author"] + "\033[0m")
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
			for repetition in range(1, testcase["repetitions"] + 1):
				testcase["repetition"] = repetition
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
							module_parameters += " --" + module + "." + parameter + "=" + str(value)

				# Setup processes
				process_source = Process("Source", docker_container=get(environment.config, ("docker_container", "source")), color=34)
				process_channel = Process("Channel", docker_container=get(environment.config, ("docker_container", "channel")), color=35)
				process_sink = Process("Sink", docker_container=get(environment.config, ("docker_container", "sink")), color=36)

				# Start processes
				process_channel.run(get(environment.config, ("run_prefix", "channel"), "") + " " + "channel_emulator --mqtt-host=" + get(environment.config, ("mqtt", "host"), "") + " --interface-source=" + get(environment.config, ("interface", "source"), "") + " --interface-sink=" + get(environment.config, ("interface", "sink"), "") + graph_file + module_parameters)
				if "sink-command" in testcase and testcase["sink-command"] != "":
					process_sink.run(get(environment.config, ("run_prefix", "sink"), "") + " " + testcase["sink-command"])

				time.sleep(1)

				if "source-command" in testcase and testcase["source-command"] != "":
					process_source.run(get(environment.config, ("run_prefix", "source"), "") + " " + testcase["source-command"])

				# Wait for source process to finish before stopping all other processes
				process_source.wait()
				process_sink.stop()
				process_channel.stop()

	# Catch keyboard interrupts
	except KeyboardInterrupt:
		# Cleanly terminate all processes
		process_source.stop()
		process_sink.stop()
		process_channel.stop()

	# Cleanup environment
	environment.cleanup()

	exit(0)

if __name__ == "__main__":
	main()
