
#pragma once

#ifndef DCPLUSPLUS_CLIENT_FLAGS_H_
#define DCPLUSPLUS_CLIENT_FLAGS_H_

class Flags
{
	public:
		typedef uint32_t MaskType;
		
		Flags() : flags(0) { }
		explicit Flags(MaskType f) : flags(f) { }
		// the flags are changed by the threads of the hubs and read by the thread of
		// the interface: the access must be atomic
		Flags(const Flags& p_other) : flags(p_other.flags.load()) { }
		Flags& operator=(const Flags& p_other)
		{
			flags.store(p_other.flags.load());
			return *this;
		}
		bool isSet(MaskType aFlag) const
		{
			return (flags.load() & aFlag) == aFlag;
		}
		bool isAnySet(MaskType aFlag) const
		{
			return (flags.load() & aFlag) != 0;
		}
		void setFlag(MaskType aFlag)
		{
			flags.fetch_or(aFlag);
		}
		void unsetFlag(MaskType aFlag)
		{
			flags.fetch_and(~aFlag);
		}
		void setFlag(MaskType aFlag, bool aValue)
		{
			if (aValue)
				setFlag(aFlag);
			else
				unsetFlag(aFlag);
		}
		void setFlags(MaskType aFlags)
		{
			flags.store(aFlags);
		}
		MaskType getFlags() const
		{
			return flags.load();
		}
	protected:
		virtual ~Flags() { }
	private:
		std::atomic<MaskType> flags;
};


#endif /* DCPLUSPLUS_CLIENT_FLAGS_H_ */
