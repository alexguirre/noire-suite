#pragma once

namespace noire
{
	template<class T>
	struct TFileTraits
	{
		static_assert("Needs TFileTraits specialization");

		/* Members

		static constexpr bool IsCollection{ ... };
		static constexpr std::uint32_t HeaderMagic{ ... };
		*/
	};
}
