
#define BUCKETS (1 << RADIX_BITS)
#define CACHE_SPACE ((CACHE_SIZE/GROUP_THREADS) * (GROUP_THREADS+1) + (CACHE_SIZE%GROUP_THREADS))
#define VALUE_SPACE (2 * CACHE_SPACE)
//To avoid bank conflict, force padding start of value space to the bank next to the start of key space
#define BANKS 32
#define KEY_SPACE ( ((VALUE_SPACE-1)/BANKS)*BANKS + (!!((VALUE_SPACE-1)%BANKS))*BANKS + 1 )
//#define CACHE_SIZE (GROUP_THREADS * ( CACHE_SPACE/(GROUP_THREADS+1) - (!!(CACHE_SIZE%GROUP_THREADS)) ))
#define WAVES (GROUP_THREADS / 32)

uint get16(inout uint array[BUCKETS/2], uint index)
{
	return (array[index / 2] >> ((index % 2) * 16)) & 0xffff;
}

void set16(inout uint array[BUCKETS / 2], uint index, uint value)
{
	array[index / 2] = (array[index / 2] & (0xffff0000 >> ((index % 2) * 16))) | (value << ((index % 2) * 16));
}

void add16(inout uint array[BUCKETS / 2], uint index, uint value)
{
	array[index / 2] += value << ((index % 2) * 16);
}

void addOne16(inout uint array[BUCKETS / 2], uint index)
{
	array[index / 2] += index % 2 ? 65536 : 1;
}

uint subOne16(inout uint array[BUCKETS / 2], uint index)
{
	bool isHighBits = index % 2;
	index /= 2;
	uint value = array[index];
	value -= isHighBits ? 65536 : 1;
	array[index] = value;
	return isHighBits ? value >> 16 : value & 0xffff;
}

RWStructuredBuffer<uint> g_inoutData : register(SHADER_RESOURCE_WRITE_SLOT0, space2);

groupshared uint globalBucketStart[BUCKETS];
groupshared uint cache[KEY_SPACE + VALUE_SPACE];
groupshared uint waveShared[WAVES][BUCKETS];
groupshared uint COUNT;
groupshared uint inputLayer;

uint getBucket(uint key, uint pass)
{
	uint offset = (pass + 1) * RADIX_BITS;
	if (32 >= offset)
	{
		uint bucket = key << (32 - offset);
		return bucket >> (32 - RADIX_BITS);
	}
	else
	{
		return key >> (32 - 32 % RADIX_BITS);
	}
}

uint paddingIndex(uint index)
{
	return (index / GROUP_THREADS) * (GROUP_THREADS + 1) + (index % GROUP_THREADS);
}

uint readKey(uint index)
{
	return cache[inputLayer * CACHE_SPACE + paddingIndex(index)];
}

uint readValue(uint index)
{
	return cache[KEY_SPACE + inputLayer * CACHE_SPACE + paddingIndex(index)];
}

void write(uint index, uint key, uint value)
{
	cache[!inputLayer * CACHE_SPACE + paddingIndex(index)] = key;
	cache[KEY_SPACE + !inputLayer * CACHE_SPACE + paddingIndex(index)] = value;
}

void sortPartition(uint pass, uint partStart, uint count, uint PART_COUNT, uint threadId)
{
#if defined INT16_COMPRESS
	uint localBucketEnd[BUCKETS/2];
#else
	uint localBucketEnd[BUCKETS];
#endif //INT16_COMPRESS
#if defined INT16_COMPRESS
	for (uint i = 0; i < BUCKETS/2; ++i)
#else
	for (uint i = 0; i < BUCKETS; ++i)
#endif //INT16_COMPRESS
	{
		localBucketEnd[i] = 0;
	}
	GroupMemoryBarrierWithGroupSync();
	for (uint i = 0; i < count; ++i)
	{
		uint key = readKey(partStart + i);
		uint bucket = getBucket(key, pass);
#if defined INT16_COMPRESS
		addOne16(localBucketEnd, bucket);
#else
		++localBucketEnd[bucket];
#endif //INT16_COMPRESS
	}

	uint waveId = threadId / 32;
#if defined INT16_COMPRESS
	for (uint i = 0; i < BUCKETS / 2; ++i)
#else
	for (uint i = 0; i < BUCKETS; ++i)
#endif //INT16_COMPRESS
	{
		localBucketEnd[i] += WavePrefixSum(localBucketEnd[i]);
		
		if (threadId % 32 == 31)
		{
#if defined INT16_COMPRESS
			uint pair = localBucketEnd[i];
			waveShared[waveId][i * 2] = pair & 0xffff;
			waveShared[waveId][i * 2 + 1] = pair >> 16;
#else
			waveShared[waveId][i] = localBucketEnd[i];
#endif //INT16_COMPRESS
		}
	}

	GroupMemoryBarrierWithGroupSync();
	if (threadId < BUCKETS)
	{
		uint sum = 0;
		for (uint k = 0; k < WAVES; ++k)
		{
			uint prefix = sum;
			sum += waveShared[k][threadId];
			waveShared[k][threadId] = prefix;
		}
	}

	GroupMemoryBarrierWithGroupSync();
	for (uint i = 0; i < BUCKETS; ++i)
	{
#if defined INT16_COMPRESS
		add16(localBucketEnd, i, waveShared[waveId][i]);
#else
		localBucketEnd[i] += waveShared[waveId][i];
#endif //INT16_COMPRESS

		if (threadId == GROUP_THREADS - 1)
		{
#if defined INT16_COMPRESS
			globalBucketStart[i] = get16(localBucketEnd, i);
#else
			globalBucketStart[i] = localBucketEnd[i];
#endif //INT16_COMPRESS
		}
	}
	
	GroupMemoryBarrierWithGroupSync();
	if (threadId < BUCKETS)
	{
		uint bucketStart = WavePrefixSum(globalBucketStart[threadId]);
		if (threadId % 32 == 31)
		{
			waveShared[waveId][0] = bucketStart + globalBucketStart[threadId];
		}
		globalBucketStart[threadId] = bucketStart;
	}
	GroupMemoryBarrierWithGroupSync();
	if (threadId < BUCKETS)
	{
		for (uint k = 0; k < waveId; ++k)
		{
			globalBucketStart[threadId] += waveShared[k][0];
		}
	}
	GroupMemoryBarrierWithGroupSync();
	
	for (uint i = count; i; --i)
	{
		uint index = partStart + i - 1;
		uint key = readKey(index);
		uint bucket = getBucket(key, pass);
#if defined INT16_COMPRESS
		write(globalBucketStart[bucket] + subOne16(localBucketEnd, bucket), key, readValue(index));
#else
		write(globalBucketStart[bucket] + --localBucketEnd[bucket], key, readValue(index));
#endif //INT16_COMPRESS
	}
}

[numthreads(GROUP_THREADS, 1, 1)]
void main(uint3 groupId : SV_GroupID, uint3 localThreadId : SV_GroupThreadId, uint3 globalThreadId : SV_DispatchThreadID, uint threadId : SV_GroupIndex)
{
	if (0 == threadId)
	{
		COUNT = g_inoutData[0];
		inputLayer = 0;
	}
	
	GroupMemoryBarrierWithGroupSync();
	if (COUNT < 2 || COUNT > CACHE_SIZE)
	{
		g_inoutData[1] = -1;
		return;
	}

	const uint PART_COUNT = (COUNT / GROUP_THREADS + !!(COUNT % GROUP_THREADS));
	const uint partStart = threadId * PART_COUNT;
	const uint localPartCount = partStart < COUNT ? min(PART_COUNT, COUNT - partStart) : 0;

	for (uint i = 0; i < PART_COUNT * 2; ++i)
	{
		uint index = i * GROUP_THREADS + threadId;
		if (index < COUNT * 2)
		{
			cache[(index%2) * KEY_SPACE + paddingIndex(index/2)] = g_inoutData[index + 2];
		}
	}

	for (uint pass = 0; pass < PASSES; ++pass)
	{
		sortPartition(pass, partStart, localPartCount, PART_COUNT, threadId);
		GroupMemoryBarrierWithGroupSync();
		if (0 == threadId)
		{
			inputLayer = !inputLayer;
		}
	}
	
	GroupMemoryBarrierWithGroupSync();
	for (uint i = 0; i < PART_COUNT * 2; ++i)
	{
		uint index = i * GROUP_THREADS + threadId;
		if (index < COUNT * 2)
		{
#if ASCENDING
			g_inoutData[index + 2] = cache[(index%2) * KEY_SPACE + inputLayer * CACHE_SPACE + paddingIndex(index/2)];
#else
			uint invIndex = COUNT - 1 - index / 2;
			g_inoutData[index + 2] = cache[(index%2) * KEY_SPACE + inputLayer * CACHE_SPACE + paddingIndex(invIndex)];
#endif //ASCENDING
		}
	}
}