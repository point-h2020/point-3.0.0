#!/usr/bin/awk -f

BEGIN {
	i = 0
	fileName=""
}
{
	fileName = FILENAME

	if ($9 ~ /Gbits/)
	{
		throughput[i] = $8
		i++
	}
	else if ($9 ~ /Mbits/)
	{
		throughput[i] = $8
		i++
	}
	else if ($8 ~ /Mbits/)
	{
		throughput[i] = $7
		i++
	}
	else if ($9 ~ /Kbits/)
	{
		throughput[i] = $7 / 1000
	}
	else if ($8 ~ /Kbits/)
	{
		throughput[i] = $7 / 1000
		i++
	}
	else if ($9 ~ /bits/)
	{
		throughput[i] = $7 / 1000000
	}
	else if ($8 ~ /bits/)
	{
		throughput[i] = $7 / 1000000
		i++
	}
}
END {
	for (it = 0; it < i; it++)
	{
		if (it == itStart)
		{
			print throughput[it] > fileName".cleaned"
		}
		else
		{
			print throughput[it] >> fileName".cleaned"
		}
	}
}
