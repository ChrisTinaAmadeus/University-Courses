# -*- coding: utf-8 -*-
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import torchvision
import torchvision.transforms as transforms

# Model structure
class Net(nn.Module):
    def __init__(self):
        # 在构造函数中，实例化不同的layer组件，并赋给类成员变量
        super(Net, self).__init__()
        self.conv1 = nn.Conv2d(3, 6, 5)
        self.pool = nn.MaxPool2d(2, 2)
        self.conv2 = nn.Conv2d(6, 16, 5)
        self.fc1 = nn.Linear(16*5*5, 120)
        self.fc2 = nn.Linear(120, 84)
        self.fc3 = nn.Linear(84, 10)

    def forward(self, x):
        # 在前馈函数中，利用实例化的组件对网络进行搭建，并对输入Tensor进行操作，并返回Tensor类型的输出结果
        x = self.pool(F.relu(self.conv1(x)))
        x = self.pool(F.relu(self.conv2(x)))
        x = x.view(-1, 16*5*5)
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = self.fc3(x)
        return x

if __name__ == '__main__':
    # 根据环境参数选取对应运行平台：cpu or gpu
    device = 'cuda:0' if torch.cuda.is_available() else 'cpu'
    print('Running Device:', device)

    # 对获取的图像数据做ToTensor()变换和归一化
    transform = transforms.Compose([transforms.ToTensor(), transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5))])

    # 利用torchvision提供的CIFAR10数据集类，实例化训练集和测试集提取类
    trainset = torchvision.datasets.CIFAR10(root='./data', train=True, download=True, transform=transform)
    testset = torchvision.datasets.CIFAR10(root='./data', train=False, download=True, transform=transform)

    # 利用torch提供的DataLoader, 实例化训练集DataLoader 和 测试集DataLoader
    Batch_size = 8
    trainLoader = torch.utils.data.DataLoader(trainset, batch_size=Batch_size, shuffle=True, num_workers=2)
    testLoader = torch.utils.data.DataLoader(testset, batch_size=Batch_size, shuffle=False, num_workers=2)

    # CIFAR10 类别内容
    classes = ('plane', 'car', 'bird', 'cat', 'deer', 'dog', 'frog', 'horse', 'ship', 'truck')


    # 卷积神经网络实例化
    net = Net().to(device)
    print(net)

    # 实例化损失函数和SGD优化子
    epochs = 3
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.SGD(net.parameters(), lr=0.001, momentum=0.9)

    # 卷积神经网络训练
    for epoch in range(epochs):
        running_loss = 0.0

        for i, data in enumerate(trainLoader, 0):
            inputs, labels = data
            # 将数据迁移到device中，如device为GPU，则将数据从CPU迁移到GPU；如device为CPU，则将数据从CPU迁移到CPU（即不作移动）
            inputs, labels = inputs.to(device), labels.to(device)

            # 清空参数的梯度值
            optimizer.zero_grad()

            # 前馈+反馈+优化
            outputs = net(inputs)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()

            # 统计训练损失
            running_loss += loss.item()

            if i % 2000 == 1999:    # 每2000 mini-batches，打印一次
                print('[%d, %5d] loss: %.3f' %
                      (epoch + 1, i + 1, running_loss / 2000))
                running_loss = 0.0

    print('Finished Training')


    # 卷积神经网络测试
    correct = 0
    total = 0
    with torch.no_grad():     # 测试阶段，停止梯度计算
        for data in testLoader:
            inputs, labels = data
            inputs, labels = inputs.to(device), labels.to(device) #

            # 前馈获得网络预测结果
            outputs = net(inputs)
            # 在预测结果中，获得预测值最大化的类别id
            _, predicted = torch.max(outputs.data, 1)

            # 统计样本个数
            total += labels.size(0)
            # 统计正确预测样本个数
            correct += (predicted == labels).sum().item()

    print('Accuracy of the network on the 10000 test inputs: %d %%' % (100 * correct / total))

    class_correct = list(0. for i in range(10))
    class_total = list(0. for i in range(10))
    with torch.no_grad():
        for data in testLoader:
            inputs, labels = data
            inputs, labels = inputs.to(device), labels.to(device)
            # 前馈获得网络预测结果
            outputs = net(inputs)
            # 在预测结果中，获得预测值最大化的类别id
            _, predicted = torch.max(outputs.data, 1)

            # 对于batch内每一个样本，获取其预测是否正确
            c = (predicted == labels).squeeze()
            # 对于batch内样本，统计每个类别的预测正确率
            for i in range(Batch_size):
                label = labels[i]
                class_correct[label] += c[i].item()
                class_total[label] += 1

    for i in range(10):
        print('Accuracy of %5s : %2d %%' % (classes[i], 100 * class_correct[i] / class_total[i]))