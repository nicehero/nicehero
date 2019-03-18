#ifndef ____MODULE____
#define ____MODULE____
#include "NoCopy.h"
#include <typeinfo>
#include "Type.h"
namespace nicehero
{
	template <typename T>
	class Module:public NoCopy
	{
		class Zero
		{
		public:
			operator int() const;
		private:
			char m_reserved[11];
		};
		class True
		{
		public:
			operator bool() const;
		private:
			char m_reserved[11];
		};
	public:
		//!获得模块指针
		static T * getInstance(void);
		//!判断模块是否成功运行过start，控制start顺序
		bool isStarted() const;

		/*! @brief 模块依赖性测查，控制start顺序


		检查本模块所依赖的模块是否成功运行start<br>
		该函数可以被子类用 bool checkStart() 函数覆盖。编译器会自动决定用哪一个checkStart。
		*/
		True checkStart();
		//!获得模块名称
		const char* getModuleName() const;
	protected:
		Module();
		virtual ~Module() = 0;

		/*! @brief 模块初始化
		初始化和构造函数的不同点在于init函数运行时可以通过g_Service获得通过main函数传入的标志
		从而通过运行参数决定该模块的初始化的行为

		构造函数会决定用是否向Service注册init方法。<br>
		只有子类用 int initial() 函数覆盖该函数后，本类的静态initial函数才会被注册到Serivce中。
		*/
		Zero initial();
		/*! @brief 模块启动



		构造函数会决定用是否向Service注册start方法。<br>
		只有子类用 int start() 函数覆盖该函数后，本类的静态start函数才会被注册到Serivce中。
		*/
		Zero start();

		/*! @brief 模块运行


		构造函数会决定用是否向Service注册run方法。<br>
		只有子类用 int run() 函数覆盖该函数后，本类的静态run函数才会被注册到Serivce中。
		*/
		Zero run();
		/*! @brief 模块停止


		构造函数会决定用是否向Service注册stop方法。<br>
		只有子类用 int stop() 函数覆盖该函数后，本类的静态stop函数才会被注册到Serivce中。
		*/
		Zero stop();

	private:
		bool			m_started;		//!< 是否准备好
		unsigned int	m_refs;			//!< 引用计数
		const char *	m_moduleName;	//!< 模块名
		static T*				m_instance;

		//!检查能否结束
		bool checkStop();

	};

	template <typename T>
	nicehero::Module<T>::~Module()
	{

	}

	template <typename T>
	nicehero::Module<T>::Module()
	{
		if (sizeof(static_cast<T *>(0)->initial()) == sizeof(int) || sizeof(static_cast<T *>(0)->initial()) == sizeof(bool))
		{
			initial();
		}
		m_moduleName = typeid(*this).name();
	}

	template <class T>
	Module<T>::Zero::operator int() const
	{
		return 0;
	}

	template <class T>
	Module<T>::True::operator bool() const
	{
		return true;
	}
	template <class T>
	bool Module<T>::isStarted() const
	{
		return m_started;
	}

	template <class T>
	typename Module<T>::True Module<T>::checkStart()
	{
		return True();
	}
	template <class T>
	typename Module<T>::Zero Module<T>::initial()
	{
		return Zero();
	}

	template <class T>
	typename Module<T>::Zero Module<T>::start()
	{
		return Zero();
	}

	template <class T>
	typename Module<T>::Zero Module<T>::run()
	{
		return Zero();
	}

	template <class T>
	typename Module<T>::Zero Module<T>::stop()
	{
		return Zero();
	}

	template <class T>
	bool Module<T>::checkStop()
	{
		return m_refs == 0;
	}

	template <class T>
	const char* Module<T>::getModuleName() const
	{
		return m_moduleName;
	}

}

#define MODULE_IMPL(T) \
namespace nicehero \
{\
	template <>    \
	T* Module<T>::m_instance = nullptr;\
	template <>		\
	T * Module<T>::getInstance() \
	{				\
		if (!m_instance)\
		{m_instance = new T();}\
		return m_instance;	\
	}\
}\

#endif // !____MODULE____

