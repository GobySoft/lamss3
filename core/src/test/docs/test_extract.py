# Copyright 2026 GobySoft, LLC
# SPDX-License-Identifier: GPL-3.0-or-later
"""The interface extractor: what it finds in C++ sources, and how it refreshes a page's tables
without losing what a person wrote in them."""

from pathlib import Path

from lamss.docs import cpp
from lamss.docs.extract import update_page

APP = r'''
#include "goby/moos/goby_moos_app.h"
class CExample : public GobyMOOSApp {};
void CExample::startup()
{
    subscribe("NAV_X", &CExample::handle_nav_x, this);
    // subscribe("COMMENTED_OUT", &CExample::handle, this);
    m_Comms.Register("NAV_Y", 0);
    std::string var = "STATUS_" + name;
    publish(var, 1.0);
    publish(is_polygon ? "OUT_POLYGON" : "OUT_SEGLIST", text.str());
    double speed = 1.5;
    m_Comms.Notify("SPEED", speed);
    m_MissionReader.GetConfigurationParam("PERIOD", period);
    m_MissionReader.GetValue("LatOrigin", lat);
}
void CExample::publish(const std::string& key, double value) {}
'''

BEHAVIOR = r'''
BHV_Example::BHV_Example(IvPDomain domain) : IvPBehavior(domain)
{
    addInfoVars("NAV_X, NAV_Y");
}
bool BHV_Example::setParam(std::string param, std::string val)
{
    if (param == "heading_variable")
        heading_variable = val;
    return true;
}
IvPFunction* BHV_Example::onRunState()
{
    double heading = getBufferDoubleVal(heading_variable, ok);
    postMessage("EXAMPLE_STATE", "running");
}
'''


def uses(text, name="Example.cpp"):
    return [(u.direction, u.name, u.transport, u.type, u.where, u.note)
            for u in cpp.scan(Path(name), text)]


def test_moos_app_calls():
    found = uses(APP)
    assert ("subscribes", "NAV_X", "MOOS", "", "handle_nav_x", "") in found
    assert ("subscribes", "NAV_Y", "MOOS", "", "Example.cpp", "") in found
    assert not any(u[1] == "COMMENTED_OUT" for u in found)
    # a ternary gives both names; a declaration of publish() is not a call
    assert ("publishes", "OUT_POLYGON", "MOOS", "string", "Example.cpp", "") in found
    assert ("publishes", "OUT_SEGLIST", "MOOS", "string", "Example.cpp", "") in found
    assert ("publishes", "?var", "MOOS", "double", "Example.cpp", "name built at run time") \
        in found
    assert ("publishes", "SPEED", "MOOS", "double", "Example.cpp", "") in found
    assert not any(u[1].startswith("?const") for u in found)
    assert ("config", "PERIOD", "", "", "Example.cpp", "") in found
    assert ("config", "LatOrigin", "", "", "Example.cpp", "global (mission file)") in found


def test_behavior_calls():
    found = uses(BEHAVIOR, "BHV_Example.cpp")
    assert ("subscribes", "NAV_X", "MOOS (helm)", "", "BHV_Example.cpp", "") in found
    assert ("subscribes", "NAV_Y", "MOOS (helm)", "", "BHV_Example.cpp", "") in found
    assert ("subscribes", "?heading_variable", "MOOS (helm)", "double", "BHV_Example.cpp",
            "name built at run time") in found
    assert ("publishes", "EXAMPLE_STATE", "MOOS (helm)", "string", "BHV_Example.cpp", "") in found
    assert ("config", "heading_variable", "", "", "BHV_Example.cpp", "") in found


PAGE = """---
title: example
source: app/
status: inferred
---

# example

<!-- BEGIN GENERATED subscribes -->
| Name | Transport | Type / format | Handler | Meaning | Notes |
|---|---|---|---|---|---|
| `NAV_X` | MOOS | double, metres | `old_handler` | x position **[code]** | |
| `GONE` | MOOS | | | removed from the code | |
<!-- END GENERATED subscribes -->

<!-- BEGIN GENERATED publishes -->
<!-- END GENERATED publishes -->

<!-- BEGIN GENERATED config -->
<!-- END GENERATED config -->
"""


def test_update_page_keeps_what_people_wrote(tmp_path):
    (tmp_path / "app").mkdir()
    (tmp_path / "app" / "Example.cpp").write_text(APP)
    page = tmp_path / "example.md"
    page.write_text(PAGE)
    text = update_page(tmp_path, page)
    assert "| `NAV_X` | MOOS | double, metres | `handle_nav_x` | x position **[code]** |" in text
    assert "GONE" not in text
    assert "| `NAV_Y` | MOOS |  | `Example.cpp` |  |  |" in text
    assert "| `var` (built at run time) | MOOS | double | `Example.cpp` |  | name built at run time |" \
        in text
    assert "| `PERIOD` |  |  |  |  |" in text
    # a second run changes nothing, which is what `lamss docs check` relies on
    page.write_text(text)
    assert update_page(tmp_path, page) == text
