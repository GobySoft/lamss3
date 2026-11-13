# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""Acomms over UDP on one machine or a network: every platform's pAcommsHandler sends to every other
one's `acomms` port."""

from missions.component.acomms import Acomms


class UdpAcomms(Acomms):
    params = dict(ip="127.0.0.1")
    ports = ["acomms"]
