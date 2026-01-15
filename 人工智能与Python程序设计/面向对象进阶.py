

class Student(object):
    # 学生信息类
    def __init__(self, name, hometown='None', phone='None'):
        '''
        学生信息类的构造函数
        参数self: 创建的实例本身
        参数name: 学生姓名
        参数hometown: 学生家乡
        参数phone: 学生电话
        '''
        self.__name       = name
        self.__hometown   = hometown
        self.__phone      = str(phone)
        self.__courses    = {}    #课程字典：课程名->课程成绩

    def input_hometown(self, hometown):
        '''
        类的方法：输入学生实例的家乡
        '''
        if self.__hometown != 'None':
            print('Warning: The existing hometown {0} is being changed into {1}.'.format(self.__hometown, hometown))
        self.__hometown   = hometown

    def input_phone(self, phone):
        '''
        类的方法：输入学生实例的电话
        '''
        if self.__phone != 'None':
            print('Warning: The existing phone {0} is being changed into {1}.'.format(self.__phone, phone))
        self.__phone   = str(phone)

    def input_course_score(self, course, score):
        '''
        类的方法：输入学生实例的指定课程的成绩
        '''
        if score >=0 and score <= 100:
            if self.__courses.get(course, -1) != -1: #如课程名不在字典key中，则返回-1，判断为假
                print('Warning: The existing {0} grade {1:.2f} is being changed into {2:.2f}.'.format(course, self.__courses.get(course, -1), score))
            self.__courses[course] = score
        else:
            print("Please input one valid course score!")

    def get_name(self):
        '''
        类的方法：获取学生实例的姓名
        '''
        return self.__name

    def get_phone(self):
        '''
        类的方法：获取学生实例的电话
        '''
        return self.__phone

    def get_hometown(self):
        '''
        类的方法：获取学生实例的家乡
        '''
        return self.__hometown

    def get_course_grade(self, course):
        '''
        类的方法：获取学生实例的指定课程的评级
        '''
        try:
            if self.__courses[course] >= 90:
                return 'A'
            elif self.__courses[course] >= 60:
                return 'B'
            else:
                return 'C'
        except:
            print("Course {0} does exist".format(course))

    def print_student_info(self):
        '''
        类的方法：获取学生实例的全部信息
        '''
        print('Name: {0}'.format(self.__name))
        print('Hometown: {0}'.format(self.__hometown))
        print('Phone: {0}'.format(self.__phone))
        if len(self.__courses) > 0:
            for course, score in self.__courses.items():
                print('Course {0}: {1:.2f}'.format(course, score))


class Graduate(Student):
    # 研究生信息类
    def __init__(self, name, hometown='None', phone='None'):
        '''
        研究生信息类的构造函数，继承自学生信息类（父类）
        参数self: 创建的实例本身
        参数name: 学生姓名
        参数hometown: 学生家乡
        参数phone: 学生电话
        '''
        super(Graduate, self).__init__(name, hometown, phone)  # 运行父类的构造函数
        self.__bachelor_major = 'None'
        self.__publication = []

    def input_paper(self, papers):
        '''
        类的方法：输入学生实例发表的文章
        '''
        if isinstance(papers, list):
            self.__publication = self.__publication + papers
        elif isinstance(papers, str):
            self.__publication.append(papers)
        else:
            print("Please organize the papers in list or str type!")

    def input_bachelor_major(self, major):
        '''
        类的方法：输入学生实例的本科专业
        '''
        self.__bachelor_major = major

    def get_publication(self):
        '''
        类的方法：获取学生实例的所有发表文章信息
        '''
        return self.__publication

    def get_bachelor_major(self):
        '''
        类的方法：获取学生实例的本科专业
        '''
        return self.__bachelor_major

    def get_course_grade(self, course):
        '''
        类的方法：获取学生实例的课程成绩评级 （覆盖父类对应方法）
        '''
        try:
            if self._Student__courses[course] >= 85:
                return 'A'
            elif self._Student__courses[course] >= 65:
                return 'B'
            else:
                return 'C'
        except:
            print("Course {0} does exist".format(course))

    def print_student_info(self):
        '''
        类的方法：获取学生实例的全部信息（覆盖父类对应方法）
        '''
        print('Name: {0}'.format(self._Student__name))
        print('Hometown: {0}'.format(self._Student__hometown))
        print('Phone: {0}'.format(self._Student__phone))
        if len(self._Student__courses) > 0:
            for course, score in self._Student__courses.items():
                print('Course {0}: {1:.2f}'.format(course, score))
        if len(self.__publication) > 0:
            print('Publication:')
            for paper in self.__publication:
                print('{0:11} {1}'.format('', paper))

# The First Sample
ZhangSan = Student(name='ZhangSan', hometown='TianJin', phone='18623443212')
ZhangSan.input_course_score('Algebra', 90)
ZhangSan.input_course_score('Python', 95)

print('ZhangSan Phone number is {0}'.format(ZhangSan.get_phone()))
print('ZhangSan {0} grade is {1}'.format('Python', ZhangSan.get_course_grade('Python')))
print('ZhangSan {0} grade is {1}'.format('English', ZhangSan.get_course_grade('English')))
ZhangSan.print_student_info()
print("\n")

# The Second Sample
LiSi = Student(name='LiSi', hometown='WuHan')
print('LiSi Phone number is {0}'.format(LiSi.get_phone()))
LiSi.input_phone(1827324834)
print('LiSi Phone number is {0}'.format(LiSi.get_phone()))
LiSi.print_student_info()
print("\n")

# The Third Sample
WangWu = Graduate(name='WangWu', hometown='Xi\'an')
WangWu.input_course_score('Computer Vision', 86)
WangWu.input_course_score('Machine Learning ', 93)
print('WangWu {0} grade is {1}'.format('Computer Vision', WangWu.get_course_grade('Computer Vision')))
WangWu.input_paper('WangWu et al. Paper_one')
WangWu.input_paper(['WangWu et al. Paper_two','WangWu et al. Paper_three'])
WangWu.print_student_info()