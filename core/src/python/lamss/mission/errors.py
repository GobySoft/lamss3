# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: LGPL-3.0-or-later


class MissionError(Exception):
    """A mission definition or command line that cannot be used; the message says why."""


class VariantConflict(MissionError):
    pass
