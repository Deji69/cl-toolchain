#include <CLASM/pch.h>
#include <CLASM/Common/String.h>
#include <cctype>

namespace CLARA {

auto ltrim(string_view subj, string_view::const_pointer str)->string_view
{
	auto p = subj.find_first_not_of(str);
	if (p == string_view::npos) return "";
	return subj.substr(p);
}

auto rtrim(string_view subj, string_view::const_pointer str)->string_view
{
	auto p = subj.find_last_not_of(str);
	if (p != string_view::npos)
		return subj.substr(0, p + 1);
	return "";
}

auto ltrimmed(string_view subj, string_view::const_pointer str)->string
{
	return string(ltrim(subj, str));
}

auto rtrimmed(string_view subj, string_view::const_pointer str)->string
{
	return string(rtrim(subj, str));
}

auto trim(string_view subj, string_view::const_pointer str)->string_view
{
	return rtrim(ltrim(subj, str));
}

auto trimmed(string_view subj, string_view::const_pointer str)->string
{
	return string(trim(subj, str));
}

auto tokenize(vector<string_view>& tokens_out, string_view subj, size_t limit, string_view::const_pointer delims, string_view::const_pointer trims)->size_t
{
	subj = trim(subj, trims);
	if (subj.empty())
		return 0;

	string_view token;
	size_t pos, i = 0;

	while (!subj.empty() && (limit == 0 || i < limit)) {
		subj = ltrim(subj, trims);
		if (subj.empty())
			break;

		pos = (i + 1) < limit ? subj.find_first_of(delims) : string_view::npos;
		token = trim(subj.substr(0, pos != string_view::npos ? pos : subj.size()));
		subj = pos != string_view::npos ? subj.substr(pos + 1) : string_view{};

		tokens_out.push_back(token);
		++i;
	}
	return i;
}

auto stringToLower(string& str)->string&
{
	std::transform(str.begin(), str.end(), str.begin(), [](string::value_type c) {
		return static_cast<unsigned char>(std::tolower(c));
	});
	return str;
}

auto stringToUpper(string& str)->string&
{
	std::transform(str.begin(), str.end(), str.begin(), [](string::value_type c) {
		return static_cast<unsigned char>(std::toupper(c));
	});
	return str;
}

auto stringLower(string str)->string
{
	return stringToLower(str);
}

auto stringUpper(string str)->string
{
	return stringToUpper(str);
}

auto stringReplace(string& subj, const string& find, const string& repl)->string&
{
	if (!subj.empty() && !find.empty()) {
		size_t pos = 0;
		while ((pos = subj.find(find, pos)) != string::npos) {
			subj.replace(pos, find.length(), repl);
			pos += repl.length();
		}
	}
	return subj;
}

auto stringReplaced(string subj, const string& find, const string& repl)->string
{
	return stringReplace(subj, find, repl);
}

auto stringBeginsWith(string_view s, string_view::value_type c)->bool
{
	return !s.empty() && s.front() == c;
}

auto stringBeginsWith(string_view s, const string& v)->bool
{
	if (v.empty()) return s.empty();
	if (s.size() >= v.size()) {
		return s.compare(0, v.size(), v) == 0;
	}
	return false;
}

auto stringEndsWith(string_view s, string_view::value_type c)->bool
{
	return !s.empty() && s.back() == c;
}

auto stringEndsWith(string_view s, const string& v)->bool
{
	if (v.empty()) return true;
	if (s.size() >= v.size()) {
		return s.compare(s.size() - v.size(), v.size(), v) == 0;
	}
	return false;
}

}
