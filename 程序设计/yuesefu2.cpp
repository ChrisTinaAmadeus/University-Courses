#include <iostream>
using namespace std;
//int step=1;
class Child
{
public:
  Child(int i)
  {
    id = i;
  }
  int getId()
  {
    return id;
  }

private:
  int id;
};

class Circle
{
public:
  Circle()
  {
    capacity=0;
    current = -1;
  }
  void in(Child *c)
  {
    children[capacity] = c;
    capacity += 1;
  }
  Child *next()
  {
    //printf("容量：%d\n", capacity);
    do
    {
      current = (current + 1) % capacity;
      //printf("%d:%d\n", step, current);
    } while (children[current] == NULL);
    return children[current];
  }
  void out(Child *c)
  {
    for(int i=0;i<capacity;i++)
    {
      if(children[i]==c)
      {
        delete children[i];
        children[i] = NULL;
        break;
      }
    }
  }

private:
  int capacity;
  int current;
  Child *children[1001];
};

int main()
{

  int n, m, i, j;
  Circle circle;
  Child *child;

  cin >> n >> m;

  for (i = 1; i <= n; i++)
  {
    child = new Child(i);
    circle.in(child);
  }

  for (i = 1; i < n; i++)
  {
    for (j = 1; j <= m; j++)
    {
      child = circle.next();
    }
    //step++;
    circle.out(child);
  }

  child = circle.next();
  cout << child->getId();
  return 0;
}