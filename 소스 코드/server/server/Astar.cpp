#include "Astar.h"

PriorityQueue::PriorityQueue() : count(0)
{
}

PriorityQueue::~PriorityQueue()
{
}

void PriorityQueue::Enq(byte v, byte l, byte i, byte f_i)
{
	int s = count++;	// ������ ��ġ
	while (s > 0) {
		int p = (s - 1) / 2;	// ���� �θ� �ε���
		if (heap[p].value < v) break;
		heap[s] = heap[p];
		s = p;
	}
	heap[s].value = v;
	heap[s].pathlen = l;
	heap[s].index = i;
	heap[s].from = f_i;
}

AstarNode PriorityQueue::Deq()
{
	AstarNode ret;
	if (count == 0) {
		ret.value = -1;
		return ret;
	}
	ret.index = heap[0].index;
	ret.value = heap[0].value;
	ret.from = heap[0].from;
	ret.pathlen = heap[0].pathlen;

	int s = 0;	// ������ �� ��尪�� ������ ��ġ
	int v = heap[--count].value;
	while (2 * s + 1 < count) {	// ������ ��ġ�� ���� �ڽ��� ���� �ִ� ��
		int min = 2 * s + 1;
		if ((min + 1 < count)
			&& (heap[min + 1].value < heap[min].value)) ++min;
		if (v < heap[min].value) break;
		heap[s] = heap[min];
		s = min;
	}
	heap[s] = heap[count];

	return(ret);
}

bool PriorityQueue::IsEmpty() const
{
	return count == 0;
}
