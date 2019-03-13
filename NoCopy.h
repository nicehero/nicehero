/*! @file
@ingroup utils
@brief �����ֹ��������
*/

#ifndef _NICEHERO_NOCOPY_H
#define _NICEHERO_NOCOPY_H

namespace nicehero
{
	/*! @brief ��ֹ��������
	@ingroup utils


	�������ֹ�����������࣬��ͨ���̳и���ʵ�ָ�Ŀ�ġ�
	*/
	class NoCopy
	{
	protected:
		NoCopy();
		~NoCopy();
	private:
		//!��ֹ��������
		NoCopy(const NoCopy &);
		//!��ֹ��������
		NoCopy & operator=(const NoCopy &);
	};

	inline NoCopy::NoCopy()
	{
	}

	inline NoCopy::~NoCopy()
	{
	}
}

#endif
