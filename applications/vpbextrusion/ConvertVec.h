#ifndef CONVERTVEC_H_
#define CONVERTVEC_H_



template <typename InType, typename OutType, 
		  unsigned int InSize = InType::num_components, 
		  unsigned int OutSize = OutType::num_components>
struct ConvertVec
{
	static void convert(InType & in, OutType & out)
	{}
};



template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 2, 2>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y())); 
	}
};

template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 2, 3>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y()), 
				static_cast<typename OutType::value_type>(0.0)); 
	}
};

template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 2, 4>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y()), 
				static_cast<typename OutType::value_type>(0.0), 
				static_cast<typename OutType::value_type>(1.0)); 
	}
};





template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 3, 2>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y())); 
	}
};

template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 3, 3>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y()), 
				static_cast<typename OutType::value_type>(in.z())); 
	}
};

template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 3, 4>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y()), 
				static_cast<typename OutType::value_type>(in.z()), 
				static_cast<typename OutType::value_type>(1.0)); 
	}
};





template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 4, 2>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()/in.w()), 
				static_cast<typename OutType::value_type>(in.y()/in.w())); 
	}
};

template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 4, 3>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()/in.w()), 
				static_cast<typename OutType::value_type>(in.y()/in.w()), 
				static_cast<typename OutType::value_type>(in.z()/in.w())); 
	}
};

template <typename InType, typename OutType>
struct ConvertVec<InType, OutType, 4, 4>
{
	static void convert(InType & in, OutType & out)
	{ 
		out.set(static_cast<typename OutType::value_type>(in.x()), 
				static_cast<typename OutType::value_type>(in.y()), 
				static_cast<typename OutType::value_type>(in.z()), 
				static_cast<typename OutType::value_type>(in.w())); 
	}
};

#endif /*CONVERTVEC_H_*/
