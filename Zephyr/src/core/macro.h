#pragma once

namespace Zephyr
{
#define DISALE_COPY_AND_MOVE(t) \
	t(const t&)            = delete;\
    t& operator=(const t&) = delete;\
    t(t&&)                 = delete;\
    t& operator=(t&&)      = delete;
}