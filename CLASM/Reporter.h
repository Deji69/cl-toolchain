#pragma once
#include <CLASM/Common.h>

namespace CLARA::CLASM {

enum class ReportType {
	Info, Warning, Error, Fatal
};

struct ReportData {
	ReportType type;
	const any& data;
};

using ReporterFunc = function<void(const ReportData&)>;

class Reporter {
public:
	Reporter() = default;

	Reporter(ReporterFunc impl) : m_impl(impl)
	{ }

	auto setImpl(ReporterFunc impl)
	{
		m_impl = impl;
	}

	auto hasImpl() const
	{
		return static_cast<bool>(m_impl);
	}

	auto report(ReportType type, const any& data) const
	{
		if (m_impl) m_impl(ReportData{type, data});
	}

	auto info(const any& data) const
	{
		report(ReportType::Info, data);
	}

	auto warn(const any& data) const
	{
		report(ReportType::Warning, data);
	}

	auto error(const any& data) const
	{
		report(ReportType::Error, data);
	}

	auto fatal(const any& data) const
	{
		report(ReportType::Fatal, data);
	}

private:
	ReporterFunc m_impl;
};

}