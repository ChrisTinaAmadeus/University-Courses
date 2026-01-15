import numpy as np
import torch, torchvision
import torch.nn as nn
import torch.optim as optim
import torch.nn.functional as F
import torchvision.transforms as transforms

# import matplotlib.pyplot as plt


def load_data(batch_size=4):
    transform = transforms.Compose(
        [transforms.ToTensor(), transforms.Normalize((0.5, 0.5, 0.5), (0.5, 0.5, 0.5))]
    )
    # If running on Windows and you get a BrokenPipeError,
    # try setting the num_worker of torch.utils.data.DataLoader() to 0.
    trainset = torchvision.datasets.CIFAR10(
        root="./data", train=True, download=True, transform=transform
    )
    trainloader = torch.utils.data.DataLoader(
        trainset, batch_size=batch_size, shuffle=True, num_workers=2
    )

    testset = torchvision.datasets.CIFAR10(
        root="./data", train=False, download=True, transform=transform
    )
    testloader = torch.utils.data.DataLoader(
        testset, batch_size=batch_size, shuffle=False, num_workers=2
    )

    classes = (
        "plane",
        "car",
        "bird",
        "cat",
        "deer",
        "dog",
        "frog",
        "horse",
        "ship",
        "truck",
    )

    return trainloader, testloader, classes


class BaselineNet(nn.Module):
    def __init__(self):
        super().__init__()
        self.conv1 = nn.Conv2d(3, 6, 5)
        self.pool = nn.MaxPool2d(2, 2)
        self.conv2 = nn.Conv2d(6, 16, 5)
        self.fc = nn.Linear(16 * 5 * 5, 10)

    def forward(self, x):
        # input x: as CIFAR-10 data: torch.Size([batch_size, 3, 32, 32])
        x = self.pool(F.relu(self.conv1(x)))
        x = self.pool(F.relu(self.conv2(x)))
        x = torch.flatten(x, 1)  # flatten all dimensions except batch
        x = self.fc(x)
        # output x: torch.Size([batch_size, classes_num])
        return x


# TODO define your net class here
class My_VGG_Net(nn.Module):  # 模仿 VGG 的 C 架构
    def __init__(self):
        super().__init__()
        self.pool = nn.MaxPool2d(2, 2)
        # Block 1
        # 输入: 3x32x32 -> 卷积 -> 64x32x32
        self.conv1_1 = nn.Conv2d(3, 64, 3, padding=1)
        self.conv1_2 = nn.Conv2d(64, 64, 3, padding=1)
        # 池化后: 64x16x16

        # Block 2
        # 输入: 64x16x16 -> 卷积 -> 128x16x16
        self.conv2_1 = nn.Conv2d(64, 128, 3, padding=1)
        self.conv2_2 = nn.Conv2d(128, 128, 3, padding=1)
        self.conv2_3 = nn.Conv2d(128, 128, 1, padding=0)
        # 池化后: 128x8x8

        # 全连接层
        self.fc1 = nn.Linear(128 * 8 * 8, 256)
        self.fc2 = nn.Linear(256, 60)
        self.fc3 = nn.Linear(60, 10)

    def forward(self, x):
        # Block 1
        x = F.relu(self.conv1_1(x))
        x = F.relu(self.conv1_2(x))
        x = self.pool(x)

        # Block 2
        x = F.relu(self.conv2_1(x))
        x = F.relu(self.conv2_2(x))
        x = F.relu(self.conv2_3(x))
        x = self.pool(x)

        # Flatten
        x = torch.flatten(x, 1)

        # FC
        x = F.relu(self.fc1(x))
        x = F.relu(self.fc2(x))
        x = self.fc3(x)
        return x

class My_Res_Net(nn.Module): # 模仿 ResNet-18 的架构
    def __init__(self):
        super().__init__()
        # 预处理层: 3 -> 64
        self.pre_conv = nn.Conv2d(3, 64, 3, padding=1)
        
        # ResBlock 1: 64 -> 64 (32×32)
        self.b1_conv1 = nn.Conv2d(64, 64, 3, padding=1)
        self.b1_conv2 = nn.Conv2d(64, 64, 3, padding=1)
        
        # ResBlock 2: 64 -> 128 (16×16)
        self.b2_conv1 = nn.Conv2d(64, 128, 3, padding=1, stride=2)
        self.b2_conv2 = nn.Conv2d(128, 128, 3, padding=1)
        self.b2_shortcut = nn.Conv2d(64, 128, 1, stride=2) # 1x1卷积调整维度
        
        # ResBlock 3: 128 -> 256 (8×8)
        self.b3_conv1 = nn.Conv2d(128, 256, 3, padding=1, stride=2)
        self.b3_conv2 = nn.Conv2d(256, 256, 3, padding=1)
        self.b3_shortcut = nn.Conv2d(128, 256, 1, stride=2) # 1x1卷积调整维度

        self.pool = nn.AdaptiveAvgPool2d((1, 1)) # 全局平均池化
        self.fc = nn.Linear(256, 10)

    def forward(self, x):
        x = F.relu(self.pre_conv(x))
        
        # Block 1
        identity = x
        out = F.relu(self.b1_conv1(x))
        out = self.b1_conv2(out)
        out += identity # 残差连接：F(x) + x
        x = F.relu(out)
        
        # Block 2
        identity = self.b2_shortcut(x) # 维度不匹配时，对输入做变换
        out = F.relu(self.b2_conv1(x))
        out = self.b2_conv2(out)
        out += identity
        x = F.relu(out)
        
        # Block 3
        identity = self.b3_shortcut(x)
        out = F.relu(self.b3_conv1(x))
        out = self.b3_conv2(out)
        out += identity
        x = F.relu(out)
        
        x = self.pool(x)
        x = torch.flatten(x, 1)
        x = self.fc(x)
        return x
    
def test(testloader, net, classes):
    correct = 0
    total = 0
    # since we're not training, we don't need to calculate the gradients for our outputs
    with torch.no_grad():
        for data in testloader:
            images, labels = data
            # calculate outputs by running images through the network
            outputs = net(images)
            # the class with the highest energy is what we choose as prediction
            _, predicted = torch.max(outputs.data, 1)
            total += labels.size(0)
            correct += (predicted == labels).sum().item()

    print(
        f"Accuracy of the network on the 10000 test images: {100 * correct // total} %"
    )

    # prepare to count predictions for each class
    correct_pred = {classname: 0 for classname in classes}
    total_pred = {classname: 0 for classname in classes}
    # again no gradients needed
    with torch.no_grad():
        for data in testloader:
            images, labels = data
            outputs = net(images)
            _, predictions = torch.max(outputs, 1)
            # collect the correct predictions for each class
            for label, prediction in zip(labels, predictions):
                if label == prediction:
                    correct_pred[classes[label]] += 1
                total_pred[classes[label]] += 1
    # print accuracy for each class
    for classname, correct_count in correct_pred.items():
        accuracy = 100 * float(correct_count) / total_pred[classname]
        print(f"Accuracy for class: {classname:5s} is {accuracy:.1f} %")


if __name__ == "__main__":
    # load data
    trainloader, testloader, classes = load_data()
    # init model
    net = My_Res_Net()
    # criterion and optimizer
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.SGD(net.parameters(), lr=0.001, momentum=0.9)

    for epoch in range(5):  # loop over the dataset multiple times

        running_loss = 0.0
        for i, data in enumerate(trainloader, 0):
            # get the inputs; data is a list of [inputs, labels]
            inputs, labels = data

            # zero the parameter gradients
            optimizer.zero_grad()

            # forward + backward + optimize
            outputs = net(inputs)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()

            # print statistics
            running_loss += loss.item()
            if i % 2000 == 1999:  # print every 2000 mini-batches
                print(
                    f"Epoch {epoch + 1:2d}, Step {i + 1:5d}. Loss: {running_loss / 2000:.3f}"
                )
                running_loss = 0.0

    print("Finished Training")

    test(testloader, net, classes)

    ckpt_path = "./cifar_net.pth"
    torch.save(net.state_dict(), ckpt_path)
