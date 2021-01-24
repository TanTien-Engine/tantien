#pragma once

#define TT_SINGLETON_DECLARATION(name) \
	public: \
		static name* Instance(); \
	private: \
		name(); \
		~name(); \
	private: \
		static name* m_instance;

#define TT_SINGLETON_DEFINITION(name) \
	name* name::m_instance = NULL; \
	name* name::Instance() \
	{ \
		if (!m_instance) { \
			m_instance = new (name)(); \
		} \
		return m_instance; \
	}