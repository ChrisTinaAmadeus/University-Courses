import torch
import torch.nn as nn
import numpy as np
import matplotlib.pyplot as plt
import os
os.environ["KMP_DUPLICATE_LIB_OK"]="TRUE"

class LinearRegression(nn.Module):
    def __init__(self, in_dim): #构造函数，需要调用nn.Mudule的构造函数
        super().__init__()       #等价于nn.Module.__init__()
        self.w = nn.Parameter(torch.randn(in_dim+1, 1))
        #self.w1 = nn.Parameter(torch.randn(in_dim + 1, 1))
        #self.w2 = nn.Parameter(torch.randn(in_dim + 1, 1))

        print("w.size()=", self.w.size())

    def forward(self, x): # x -> y
        x = torch.cat([x, torch.ones((x.shape[0],1))], dim = 1)
        y = x.matmul(self.w)
        return y



def testForwardLR(in_dim, data_size = 2):
    layer = LinearRegression(in_dim)
    input=torch.randn(data_size,in_dim)

    output=layer(input)  #前向传播 执行forward() ,  __call__
    output=layer.forward(input)

    print(output)
    for name, parameter in layer.named_parameters():
        print(name, parameter)

testForwardLR(2,2)




class Linear_Model():
    def __init__(self, in_dim):
        """
        创建模型和优化器，初始化线性模型和优化器超参数
        """
        self.learning_rate = 0.01
        self.epoches = 1000
        self.model = LinearRegression(in_dim) #torch.nn.Linear(in_dim,1)
        self.optimizer = torch.optim.SGD(self.model.parameters(), lr=self.learning_rate)
        self.loss_function = torch.nn.MSELoss()

    def train(self, x, y, if_plot = False):
        """
        训练模型并保存参数
        输入:
            model_save_path: saved name of model
            x: 训练数据
            y: 回归真值
        返回:
            losses: 所有迭代中损失函数值
        """
        losses = []
        for epoch in range(self.epoches):
            prediction = self.model(x) # forward, __call__  input->output

            loss = self.loss_function(prediction, y)

            self.optimizer.zero_grad()
            loss.backward() #gradient
            self.optimizer.step() #update

            losses.append(loss.item())

            if epoch % 50 == 0:
                print("epoch: {}, loss is: {}".format(epoch, loss.item()))

        if x.shape[1]==1 and if_plot:
            plt.figure()
            plt.scatter(x.numpy(), y.numpy())
            plt.plot(x.numpy(), prediction.numpy(), color="r")
            plt.show()

        return losses


    def test(self, x, y, if_plot = False):
        """
        用保存或训练好的模型做测试
        输入:
            model_path: 训练好的模型的保存路径, e.g., "linear.pth"
            x: 测试数据
            y: 测试数据的回归真值
        返回:
            prediction: 测试数据的预测值
        """
        prediction = self.model(x)
        testMSE = self.loss_function(prediction, y)

        if if_plot and x.shape[1]==1 and if_plot:
            plt.figure()
            plt.scatter(x.numpy(), y.numpy())
            plt.plot(x.numpy(), prediction.numpy(), color="r")
            plt.show()

        return prediction, testMSE


def create_linear_data(data_size, in_dim, if_plot = False):
    """
    为线性模型生成数据
    输入:
        data_size: 样本数量
    返回:
        x_train: 训练数据
        y_train: 训练数据回归真值
        x_test: 测试数据
        y_test: 测试数据回归真值
    """
    np.random.seed(426)
    torch.manual_seed(426)
    #torch.cuda.manual_seed(426)

    x = torch.Tensor(data_size, in_dim).uniform_(1,10)

    w_true = torch.Tensor(in_dim, 1).uniform_(-5,5)
    #map_true = torch.tensor([[1.5],[-5.],[3.]], dtype=torch.float32)
    print('w真值:{}'.format(w_true))

    y = x.mm(w_true) + 10. + torch.FloatTensor(data_size, 1).normal_(0,10) #torch.randn(x.size())

    shuffled_index = np.random.permutation(data_size)
    #shuffled_index = torch.from_numpy(shuffled_index).long()
    x = x[shuffled_index]
    y = y[shuffled_index]
    split_index = int(data_size * 0.7)
    x_train = x[:split_index]
    y_train = y[:split_index]
    x_test = x[split_index:]
    y_test = y[split_index:]

    if if_plot and in_dim == 1:
        plt.figure()
        plt.scatter(x_train.numpy(),y_train.numpy())
        plt.show()
    return x_train, y_train, x_test, y_test

# 模型验证
#testLRmodel(3)

# 生成数据
data_size = 100
in_dim = 2
x_train, y_train, x_test, y_test = create_linear_data(data_size, in_dim, if_plot=False)

# 线性回归模型实例化
linear = Linear_Model(in_dim)
# 模型训练
losses = linear.train(x_train, y_train)

if_plot = False # set to True if you want to plot
if if_plot:
    plt.figure()
    plt.scatter(np.arange(len(losses)), losses, marker='o', c='green')
    plt.savefig('loss.jpg')
    plt.show()
# 模型测试
prediction, testMSE = linear.test(x_test, y_test)
print('测试集上MSE损失值:{}'.format(testMSE))


print('===============================')
for name, parameter in linear.model.named_parameters(): #named_parameters()  parameters()
    print(name, parameter)

