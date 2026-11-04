// Copyright 2016-2026 Massachusetts Institute of Technology
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef _WIN32
#pragma warning(disable : 4786)
#pragma warning(disable : 4503)
#endif
#ifndef IVPBUILD_ZAIC_VECTOR_HEADER
#define IVPBUILD_ZAIC_VECTOR_HEADER

#include "IvPDomain.h"
#include "IvPFunction.h"
#include <string>
#include <vector>

class PDMap;

namespace lamss
{
namespace autonomy
{

class ZAIC_Vector
{
  public:
    ZAIC_Vector(IvPDomain g_domain, const std::string& g_varname);
    virtual ~ZAIC_Vector(){};

    void setDomainVals(std::vector<double> v);
    void setRangeVals(std::vector<double> v);
    double getParam(std::string);
    void print();

    std::vector<std::string> getErrors() { return (m_errors); };
    bool hasErrors() { return (m_errors.size() != 0); };
    void clearErrors() { m_errors.clear(); };

    std::string getWarnings() { return (m_warnings); };
    unsigned int getWarningCount();
    bool hasWarnings() { return (m_warnings.size() != 0); };
    void clearWarnings() { m_warnings = ""; };

    unsigned int size() { return (m_domain_pts); };

    IvPFunction* extractOF();
    IvPFunction* extractIvPFunction() { return (extractOF()); };

  protected:
    bool stateOK();
    void convertValues();
    void addWarning(std::string);
    void addError(std::string);
    PDMap* setPDMap();

    IvPBox* buildBox(unsigned int ix_low, double val_low, unsigned int ix_hgh, double val_hgh);

  protected: // Parameters
    std::vector<double> m_domain_vals;
    std::vector<double> m_range_vals;

    double m_minutil;
    double m_maxutil;

  protected: // State values
    std::vector<unsigned int> m_idomain_vals;
    std::vector<double> m_irange_vals;
    std::vector<std::string> m_errors;

    unsigned int m_warning_count;
    unsigned int m_domain_pts;
    unsigned int m_domain_ix;
    double m_domain_high;
    double m_domain_low;
    double m_domain_delta;

    bool m_domain_ok;
    std::string m_warnings;
    IvPDomain m_ivp_domain;
};
} // namespace autonomy
} // namespace lamss

#endif
