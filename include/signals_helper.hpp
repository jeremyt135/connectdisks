#ifndef SIGNALS_HELPER_H
#define SIGNALS_HELPER_H

#include <boost/signals2.hpp>

#define CAT_(x, y) x##y
#define CAT(x, y) CAT_(x,y)
#define SIGNAL(x) CAT(x, Signal)
#define SLOT(x) CAT(x, Handler)

#define ADD_SIGNAL(sig, name, ret, ...)\
public:\
	using SIGNAL(sig) = boost::signals2::signal<ret(__VA_ARGS__)>;\
	using SLOT(sig) = SIGNAL(sig) :: slot_type;\
private: \
	SIGNAL(sig) name;\
public: \
	boost::signals2::connection CAT(connect, SLOT(sig))(const SLOT(sig) & handler)\
	{\
		return name . connect(handler);\
	}\

#endif