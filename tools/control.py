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
import flowemu.control as flowemu

flowemuctl = flowemu.Control()

# Get start timestamp
timestamp_start = perf_counter()

# Run experiment
timestamp = timestamp_start
for i in np.linspace(0, 100, 11):
	module = "fifo_queue"
	parameter = "buffer_size"

	# Set FIFO queue buffer size
	print(f"[{perf_counter() - timestamp_start}] Set FIFO queue buffer size to {round(i)} packets!")
	flowemuctl.setModuleParameter(module, parameter, round(i))

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
