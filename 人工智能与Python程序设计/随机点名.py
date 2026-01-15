import xlrd
import random
import datetime

def select_student(students):
    lucky_number = random.randint(0, len(students)-1)
    return students[lucky_number]


def read_excel():
    info = xlrd.open_workbook(filename='students.xls').sheet_by_index(0)  # 打开文件
    student_rows = []

    Start_Line_Num = 1
    for i in range(Start_Line_Num, info.nrows):
        row = list(info.row_values(i))
        if len(str(row[0])) < 1:
            continue
        #print('[', row[1:3], ']')
        student_rows.append(row[1:3])
    #print(len(student_rows))
    return student_rows


if __name__ == '__main__':
    students = read_excel()
    random.seed(datetime.datetime.now().strftime("%y%m%d%h%m%s"))
    #random.seed()
    print(len(students))
    for i in range(len(students)):
        print(i, students[i])
    while True:
        C = input("Continue? [y/n]")
        if C != 'y':
            break
        print()
        print(select_student(students))
        print()
