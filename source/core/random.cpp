#include "random.h"
#include <random>

namespace Core
{
	namespace Random
	{
		// Global seed should be used unless you have a specific use cse
		std::mt19937 g_globalGenerator;

		void ResetGlobalSeed()
		{
			std::random_device device;
			g_globalGenerator = std::mt19937(device());
		}

		void SetGlobalSeed(uint32_t seed)
		{
			g_globalGenerator= std::mt19937(seed);
		}

		float GetFloat(float minval, float maxval)
		{
			std::uniform_real_distribution<float> dist(minval, maxval);
			return dist(g_globalGenerator);
		}

		int GetInt(int minval, int maxval)
		{
			std::uniform_int_distribution dist(minval, maxval);
			return dist(g_globalGenerator);
		}
	}
}