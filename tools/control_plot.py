#!/usr/bin/env python3

# FlowEmu - Flow-Based Network Emulator
# Copyright (c) 2022 Institute of Communication Networks (ComNets),
#                    Hamburg University of Technology (TUHH),
#                    https://www.tuhh.de/comnets
# Copyright (c) 2022 Daniel Stolpmann <daniel.stolpmann@tuhh.de>
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

from time import sleep, perf_counter
import numpy as np
import matplotlib.pyplot as plt
import flowemu.control as flowemu

from typing import List


flowemuctl = flowemu.Control()

# Create data arrays
buffer_size_x: List[float] = []
buffer_size_y: List[float] = []
queue_length_x: List[float] = []
queue_length_y: List[float] = []

# Warm up
flowemuctl.setModuleParameter("fifo_queue", "buffer_size", str(0))
sleep(1)

# Get start timestamp
timestamp_start = perf_counter()

# Register statistic handlers
@flowemuctl.onModuleStatistic("fifo_queue", "queue_length")
def onFifoQueueQueueLength(value: str) -> None:
	timestamp = perf_counter() - timestamp_start
	value_int = round(float(value))
	print(f"[{timestamp}] Queue length: {value_int} packets")
	queue_length_x.append(timestamp)
	queue_length_y.append(value_int)

@flowemuctl.onModuleStatistic("fifo_queue", "buffer_size")
def onFifoQueueBufferSize(value: str) -> None:
	timestamp = perf_counter() - timestamp_start
	value_int = round(float(value))
	print(f"[{timestamp}] FIFO queue buffer size: {value_int} packets")
	buffer_size_x.append(timestamp)
	buffer_size_y.append(value_int)

# Run experiment
timestamp = timestamp_start
for i in np.linspace(0, 100, 11):
	module = "fifo_queue"
	parameter = "buffer_size"

	# Set FIFO queue buffer size
	print(f"[{perf_counter() - timestamp_start}] Set FIFO queue buffer size to {round(i)} packets!")
	flowemuctl.setModuleParameter(module, parameter, str(round(i)))

	# Increase timestamp
	timestamp += 10

	# Wait with high precision till timestamp is reached
	while True:
		diff = timestamp - perf_counter()
		if diff < 10**-6:
			break

		sleep(diff * 0.9)

# Close connection to FlowEmu
flowemuctl.close()

# Extend buffer size plot to end of experiment
buffer_size_x.append(perf_counter() - timestamp_start)
buffer_size_y.append(buffer_size_y[-1])

# Plot data
plt.step(buffer_size_x, buffer_size_y, where="post", label="Buffer Size")
plt.step(queue_length_x, queue_length_y, where="post", label="Queue Length")

# Label axis
plt.title("FIFO Queue")
plt.xlabel("Time [s]")
plt.ylabel("Buffer Size / Queue Length [packets]")
plt.legend()

# Show plot
plt.tight_layout()
plt.show() # type: ignore
