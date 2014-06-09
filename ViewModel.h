#pragma once
namespace StarterKit
{

	ref class ViewModel sealed
	{
	private:
		int m_scoreUser;
		int m_scoreMachine;
	public:
		ViewModel();
		event Windows::Foundation::TypedEventHandler<Object^, Platform::String^>^ PropertyChanged;

		property int ScoreUser
		{
			int get()
			{
				return m_scoreUser;
			}

			void set(int value)
			{
				if (m_scoreUser != value)
				{
					m_scoreUser = value;
					PropertyChanged(this, L"ScoreUser");
				}
			}
		};

		property int ScoreMachine
		{
			int get()
			{
				return m_scoreMachine;
			}

			void set(int value)
			{
				if (m_scoreMachine != value)
				{
					m_scoreMachine = value;
					PropertyChanged(this, L"ScoreMachine");
				}
			}
		};
	};
}
