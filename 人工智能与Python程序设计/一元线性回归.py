import numpy as np
import matplotlib.pyplot as plt


class LinearRegression(object):
    def __init__(self, learning_rate=0.01, max_iter=100, seed=None):
        """
        一元线性回归类的构造函数：
        参数 学习率：learning_rate
        参数 最大迭代次数：max_iter
        参数 seed：产生随机数的种子
        从正态分布中采样w和b的初始值
        """
        np.random.seed(seed)
        self.lr = learning_rate
        self.max_iter = max_iter
        self.w = np.random.normal(1, 0.1)
        self.b = np.random.normal(1, 0.1)
        self.loss_arr = []

    def fit(self, x, y):
        """
        类的方法：训练函数
        参数 自变量：x
        参数 因变量：y
        返回每一次迭代后的损失函数
        """
        for i in range(self.max_iter):
            print("iter ", i)
            self.__train_step(x, y)
            y_pred = self.predict(x)
            self.loss_arr.append(self.loss(y, y_pred))
            print("loss = ", self.loss_arr[-1])

    def __f(self, x, w, b):
        '''
        类的方法：计算一元线性回归函数在x处的值
        '''
        #print("entering __f()")
        #print(x.shape)
        #print("leaving __f()")
        return x * w + b

    def predict(self, x):
        '''
        类的方法：预测函数
        参数：自变量：x
        返回：对x的回归值
        '''
        y_pred = self.__f(x, self.w, self.b)
        #print('inside predict(): y_pred size:', y_pred.shape)
        return y_pred# y^hat

    def loss(self, y_true, y_pred):
        '''
        类的方法：计算损失
        参数 真实因变量：y_true
        参数 预测因变量：y_pred
        返回：MSE损失
        '''
        #print("y_true, ", y_true.shape)
        #print("y_pred, ", y_pred.shape)
        return np.mean((y_true - y_pred) ** 2) #MSE

    def __calc_gradient(self, x, y):
        '''
        类的方法：分别计算对w和b的梯度
        '''

        d_w = np.mean(2 * (x * self.w + self.b - y) * x)
        d_b = np.mean(2 * (x * self.w + self.b - y))

        print("d_w", d_w)

        print("d_b", d_b)

        return d_w, d_b

    def __train_step(self, x, y):
        '''
        类的方法：单步迭代，即一次迭代中对梯度进行更新
        '''
        d_w, d_b = self.__calc_gradient(x, y)
        self.w = self.w - self.lr * d_w
        self.b = self.b - self.lr * d_b
        return self.w, self.b

def show_data(x, y, w=None, b=None):
    plt.scatter(x, y, marker='.')
    if w is not None and b is not None:
        plt.plot(x, w * x + b, c='red')
    plt.show()


# data generation
np.random.seed(272)
data_size = 100

x = np.random.uniform(low=1.0, high=10.0, size=data_size)
y = x * 20 + 10 + np.random.normal(loc=0.0, scale=10.0, size=data_size)

print(x.shape, y.shape)
#print(x, y)

# train / test split
shuffled_index = np.random.permutation(data_size)
#print("shuffled_index", shuffled_index)
#===============================================
#print("before, ", x[0], x[62])
x = x[shuffled_index]
#x[[62,24,85]]---[ x[62], x[24], x[85] ]
#print("after, ", x[0], x[62])
y = y[shuffled_index]
split_index = int(data_size * 0.7)


x_train = x[:split_index]#0-69
y_train = y[:split_index]

x_test = x[split_index:]#70-99
y_test = y[split_index:]

#print("----------------------------------")
#print(x_train.shape, y_train.shape, x_test.shape, y_test.shape)

#print("----------------------------------")
# train the liner regression model
regr = LinearRegression(learning_rate=0.01, max_iter=100, seed=0)
regr.fit(x_train, y_train)

print('w: \t{:.3}'.format(regr.w))
print('b: \t{:.3}'.format(regr.b))
show_data(x, y, regr.w, regr.b)

# plot the evolution of cost
plt.scatter(np.arange(len(regr.loss_arr)), regr.loss_arr, marker='o', c='green')
plt.show()
