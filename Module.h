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
		//!���ģ��ָ��
		static T * getInstance(void);
		//!�ж�ģ���Ƿ�ɹ����й�start������start˳��
		bool isStarted() const;

		/*! @brief ģ�������Բ�飬����start˳��


		��鱾ģ����������ģ���Ƿ�ɹ�����start<br>
		�ú������Ա������� bool checkStart() �������ǡ����������Զ���������һ��checkStart��
		*/
		True checkStart();
		//!���ģ������
		const char* getModuleName() const;
	protected:
		Module();
		virtual ~Module() = 0;

		/*! @brief ģ���ʼ��
		��ʼ���͹��캯���Ĳ�ͬ������init��������ʱ����ͨ��g_Service���ͨ��main��������ı�־
		�Ӷ�ͨ�����в���������ģ��ĳ�ʼ������Ϊ

		���캯����������Ƿ���Serviceע��init������<br>
		ֻ�������� int initial() �������Ǹú����󣬱���ľ�̬initial�����Żᱻע�ᵽSerivce�С�
		*/
		Zero initial();
		/*! @brief ģ������



		���캯����������Ƿ���Serviceע��start������<br>
		ֻ�������� int start() �������Ǹú����󣬱���ľ�̬start�����Żᱻע�ᵽSerivce�С�
		*/
		Zero start();

		/*! @brief ģ������


		���캯����������Ƿ���Serviceע��run������<br>
		ֻ�������� int run() �������Ǹú����󣬱���ľ�̬run�����Żᱻע�ᵽSerivce�С�
		*/
		Zero run();
		/*! @brief ģ��ֹͣ


		���캯����������Ƿ���Serviceע��stop������<br>
		ֻ�������� int stop() �������Ǹú����󣬱���ľ�̬stop�����Żᱻע�ᵽSerivce�С�
		*/
		Zero stop();

	private:
		bool			m_started;		//!< �Ƿ�׼����
		unsigned int	m_refs;			//!< ���ü���
		const char *	m_moduleName;	//!< ģ����
		static T*				m_instance;

		//!����ܷ����
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

