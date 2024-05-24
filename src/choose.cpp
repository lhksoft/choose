/* ******************************************************************************
 * choose.cpp
 * (w) Laurens Koehoorn, LhK-Soft, 2024-05-20
 * This program is as is, it's free and uses the GNU GPLv3 license
 * 
 * compile/link using :
 *   g++ ./choose.cpp -o ./choose.exe
 * or
 *   g++ ./choose.cpp -o ./choose
 * 
 * based on CHOICE.C of ReactOs, by Eric Kohl, Paolo Pantaleo and Magnus Olsen
 * <https://github.com/reactos/reactos/blob/master/base/shell/cmd/choice.c>
 * 
 * KeyPress class/routines based on code in this message :
 * <https://cplusplus.com/forum/general/5304/#msg23940>
 * 
 * 'GetTickCount' based at code in this message :
 * <https://stackoverflow.com/a/24959236>
 * 
 * This is a tool to be used in shell-scripts, works like select but instead
 * entering numbers for menu-choices, one enter any predefined alpanumeric charater
 * ****************************************************************************** */

// #define _BSD_SOURCE // required for 'setlinebuf()'
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h> // for atoi
#include <ctype.h> // for isalnum
#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include <cctype> // for using toupper 
#include <chrono>
#include <string.h> // for memcpy, strchr
#include <unistd.h>
#include <termios.h>
#include <poll.h>

// simulation of Windows GetTickCount()
unsigned long long
GetTickCount()
{
    using namespace std::chrono;
    return static_cast<unsigned long long>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

// Clock built upon Windows GetTickCount()
struct TickCountClock
{
  typedef unsigned long long                       rep;
  typedef std::milli                               period;
  typedef std::chrono::duration<rep, period>       duration;
  typedef std::chrono::time_point<TickCountClock>  time_point;
  static const bool is_steady =                    true;

  static time_point now() noexcept
  {
    return time_point(duration(GetTickCount()));
  }
};

class CKey {
  public:
    CKey();
    CKey(const CKey&) = delete;
    CKey(CKey&&) = delete;
    virtual ~CKey();

    static char kbHit(int timeout=-1);

  protected:
    bool        b_Initialized;
    termios     t_Settings;

    bool        Initialise(void);
    void        Finalize(void);
    char        _kbHit(int timeout);
};

class COpt {
  public:
    COpt();
    COpt(const COpt&) = delete;
    COpt(COpt&&) = delete;
    virtual ~COpt();

    bool        Analize(int, char**);
    void        Debug(void);

    void        ShowHelp(void);
    int         KeyInOpt(char c);
    void        ShowPrompt(void); // displays promt and/or choices, followed by a '?'
    void        Cleanup(void); // will do some cleanup upon termination, not called from destructor

  protected:
    bool        b_showHelp, b_hideChoices, b_noCase;
    int         i_timeout;

    std::string s_choices;
    std::string s_prompt;
    char        c_default;

  public:
    bool        NeedHelp(void)        { return b_showHelp; }
    char        GetDefaultKey(void)   { return c_default; }
    int         GetTimeout(void)      { return i_timeout; }
};

/* ******************************************************************************
 * class COpt
 */
COpt::COpt() : s_choices(), s_prompt()
{
  c_default     = 0;
  b_showHelp    = false;
  b_hideChoices = false;
  b_noCase      = true;
  i_timeout     = -1; // infinite
}
COpt::~COpt()
{
}
bool COpt::Analize(int argc, char **argv)
{
  bool bRet=true;
  int cc;
  long l;

  while (bRet && ((cc=getopt(argc, argv, "hnsc:t:d:m:")) != -1))
  {
    switch (cc)
    {
    case 'c':
      s_choices = optarg;
      break;
    case 'd':
      c_default = optarg[0];
      break;
    case 'h':
      b_showHelp = true;
      break;
    case 'm':
      s_prompt = optarg;
      break;
    case 'n':
      b_hideChoices = true;
      break;
    case 's':
      b_noCase = false;
      break;
    case 't':
      // this value is 'seconds'
      l = atol(optarg);
      i_timeout = ((l>=0) && (l<=9999)) ? static_cast<int>(l) : -1;
      break;
    default:
      bRet = false;
      break;
    }
  }
  if (!bRet)
    return false;

  // Verification
  // ============
  if (s_choices.empty())
    s_choices = "YN";
  // verify choices
  for (char & c : s_choices)
  {
    if (isalnum(c) && !isspace(c))
      ; // ok
    else
    {
      std::cerr << "Invalid choices, should be in range of [0..9][A..Z][a..z]." << std::endl;
      return false;
    }
  }
  // verify c_default
  if (c_default)
  {
    if (isalnum(c_default) && !isspace(c_default))
      ; // ok
    else
    {
      std::cerr << "Invalid default-value, should be in range of [0..9][A..Z][a..z]." << std::endl;
      return false;
    }
    // check it is valid in our choices
    bool b=false;
    for (char & c : s_choices)
    {
      if ((b_noCase && (std::toupper(c) == std::toupper(c_default))) || (!b_noCase && (c == c_default)))
      {
        b = true;
        break;
      }
    }
    if (!b)
    {
      std::cerr << "Invalid default-value, should be in range of [" << s_choices << "], but given '" << c_default << "'." << std::endl;
      return false;
    }
  }
  if (i_timeout >= 0)
  {
    if (!c_default)
    {
      std::cerr << "Timeout needs a default-value." << std::endl;
      return false;
    }
  }

  return true;
}
void COpt::Debug()
{
  std::cerr << "b_showHelp     : " << ((b_showHelp) ? "true" : "false") << std::endl;
  std::cerr << "b_hideChoices  : " << ((b_hideChoices) ? "true" : "false") << std::endl;
  std::cerr << "b_noCase       : " << ((b_noCase) ? "true" : "false") << std::endl;
  std::cerr << "i_timeout      : " << i_timeout << std::endl;
  std::cerr << "s_choices      : " << s_choices << std::endl;
  std::cerr << "s_prompt       : " << s_prompt << std::endl;
  std::cerr << "c_default      : " << c_default << std::endl;
}
void COpt::ShowHelp()
{
  std::cerr << "CHOOSE [-c choices] [-n] [-s] [-t timeout -d choice] [-m text]" << std::endl;
  std::cerr << " (w)2024 (c)Laurens Koehoorn, LhK-Soft - GNU GPLv3 licence." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Description:" << std::endl;
  std::cerr << "    This tool allows users to select one item from a list" << std::endl;
  std::cerr << "    of choices and returns the index of the selected choice." << std::endl;
  std::cerr << "    This tool is based on CHOICE.EXE from ReactOS." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Parameter List:" << std::endl;
  std::cerr << "   -c    choices       Specifies the list of choices to be created." << std::endl;
  std::cerr << "                       Default list is \"YN\"." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   -n                  Hides the list of choices in the prompt." << std::endl;
  std::cerr << "                       The message before the prompt is displayed" << std::endl;
  std::cerr << "                       and the choices are still enabled." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   -s                  Enables case-sensitive choices to be selected." << std::endl;
  std::cerr << "                       By default, the utility is case-insensitive." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   -t    timeout       The number of seconds to pause before a default" << std::endl;
  std::cerr << "                       choice is made. Acceptable values are from 0 to" << std::endl;
  std::cerr << "                       9999. If 0 is specified, there will be no pause" << std::endl;
  std::cerr << "                       and the default choice is selected." << std::endl;
  std::cerr << "                       If no timeout given, CHOOSE will wait forever" << std::endl;
  std::cerr << "                       for input." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   -d    choice        Specifies the default choice after nnnn seconds." << std::endl;
  std::cerr << "                       Character must be in the set of choices specified" << std::endl;
  std::cerr << "                       by -c option and must also specify nnnn with -t." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   -m    text          Specifies the message to be displayed before" << std::endl;
  std::cerr << "                       the prompt. If not specified, the utility" << std::endl;
  std::cerr << "                       displays only a prompt." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   -h                  Displays this help message." << std::endl;
  std::cerr << std::endl;
  std::cerr << "   NOTE:" << std::endl;
  std::cerr << "   The return-value is set to the index of the key that was selected" << std::endl;
  std::cerr << "   from the set of choices. The first choice listed returns a value of 1," << std::endl;
  std::cerr << "   the second a value of 2, and so on." << std::endl;
  std::cerr << "   If the user presses a key that is not a valid choice, the tool" << std::endl;
  std::cerr << "   sounds a warning beep (not working in *nix). If tool detects an" << std::endl;
  std::cerr << "   error condition, it returns a value of 255." << std::endl;
  std::cerr << "   When used in shell scripts, test the return-value in decreasing order." << std::endl;
  std::cerr << "   Upon successfull completion (return >= 1), the promptline is cleared." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Examples:" << std::endl;
  std::cerr << "   CHOOSE -h" << std::endl;
  std::cerr << "   CHOOSE -c YNC -m \"Press Y for Yes, N for No or C for Cancel.\"" << std::endl;
  std::cerr << "   CHOOSE -t 10 -c ync -s -d y" << std::endl;
  std::cerr << "   CHOOSE -c ab -m \"Select a for option 1 and b for option 2.\"" << std::endl;
  std::cerr << "   CHOOSE -c ab -n -m \"Select a for option 1 and b for option 2.\"" << std::endl;
}
int COpt::KeyInOpt(char c)
{
  int cur=-1;
  if (isalnum(c) && !isspace(c))
  {
    bool b=false;
    for (char & cc : s_choices)
    {
      cur++;
      if ((b=((b_noCase && (std::toupper(cc) == std::toupper(c))) || (!b_noCase && (cc == c)))))
        break;
    }
    if (!b) cur = -1;
  }
  return cur;
}
void COpt::ShowPrompt() // displays promt and/or choices, followed by a '?'
{
/*
  std::string txt;
  if (!s_prompt.empty())
    txt += s_prompt;
  if (!b_hideChoices)
  {
    txt += "[";
    txt += s_choices[0];
    for (unsigned i=1; i<s_choices.length(); i++)
    {
      txt += ",";
      txt += s_choices[i];
    }
    txt += "]";
  }
  txt += "? ";
  std::cerr << txt << std::flush;
*/
  if (!s_prompt.empty())
    std::cerr << s_prompt;
  if (!b_hideChoices)
  {
    std::cerr << "[" << s_choices[0];
    for (unsigned i=1; i<s_choices.length(); i++)
    {
      std::cerr << "," << s_choices[i];
    }
    std::cerr << "]?";
  }
  std::cerr.flush();
  // in *nix mostly this prompt is not shown, therefore sleep some time
  usleep(100);
}
void COpt::Cleanup()
{
  if (!s_prompt.empty() || !b_hideChoices)
    std::cerr << "\x1b[2K\r" << std::flush;
}

/* ******************************************************************************
 * class CKey
 */
CKey::CKey() : t_Settings()
{
  b_Initialized = false;
}
CKey::~CKey()
{
  Finalize();
}
bool CKey::Initialise()
{
  if (!b_Initialized)
  {
    b_Initialized = (bool)isatty( STDIN_FILENO );
    if (b_Initialized)
      b_Initialized = (0 == tcgetattr( STDIN_FILENO, &t_Settings ));
    if (b_Initialized)
      std::cin.sync_with_stdio();
  }

  return b_Initialized;
}
void CKey::Finalize()
{
  if (b_Initialized)
    tcsetattr( STDIN_FILENO, TCSANOW, &t_Settings );
  b_Initialized = false;
}
char CKey::_kbHit(int timeout)
{
  char c=0;
  termios settings;
  if (!b_Initialized)
    if (!Initialise())
      return 1;

  memcpy(&settings, &t_Settings, sizeof(termios));
  tcflush( STDIN_FILENO, TCIOFLUSH );

  settings.c_lflag &= ~(ICANON | ECHO);
  tcsetattr( STDIN_FILENO, TCSANOW, &settings );
  setbuf( stdin, NULL );

  pollfd pls[ 1 ];
  pls[ 0 ].fd     = STDIN_FILENO;
  pls[ 0 ].events = POLLIN | POLLPRI;

  int i = poll( pls, 1, timeout );
  if (i > 0)
    c = std::cin.get();
  else if (i == 0)
    c = 0; // timeout
  else
    c = 1; // error
  tcflush( STDIN_FILENO, TCIOFLUSH );

  settings.c_lflag |= (ICANON | ECHO);
  tcsetattr( STDIN_FILENO, TCSANOW, &settings );
  setlinebuf( stdin );

  return c;
}
//static
char CKey::kbHit(int timeout)
{
  CKey* pKey=new CKey();
  char ret=pKey->_kbHit(timeout);
  delete pKey;

  return ret;
}

//---------------------------------------------------------------------------

/* ************************************************************************************************ */
int main(int argc, char **argv)
{
  COpt *pOpt = new COpt();
  if (!pOpt->Analize(argc, argv))
  {
    delete pOpt;
    return 255;
  }
  else
  {
//    pOpt->Debug();
    if (pOpt->NeedHelp())
    {
      pOpt->ShowHelp();
      delete pOpt;
      return 0;
    }
  }

  // then if all still okay, do it !
  pOpt->ShowPrompt();

  int nErrLevel=-1;
  char key;

  if (pOpt->GetTimeout() < 0)
  { // not using a timeout
    while (true)
    {
      key = CKey::kbHit();
      /*
        if (key == 0)
          Timeout
        else if (key == 1)
          Error
        else if (key == 27)
          Special key
        else if ((c>=32) && (c<=126))
          printable (valid)
        else
          Invalid key
      */
      if ( ((key>=33) && (key<=126)) && ((nErrLevel=pOpt->KeyInOpt(key)) >= 0) )
        break;
      else if (key == 1)
      {
        nErrLevel = 255;
        break;
      }
      else
        std::cerr << '\a' << std::flush; // sound bell -- doesn't work in *nix :(
    }
  }
  else
  { // using timeout
    auto t0 = TickCountClock::now();
    int nTimeout = (pOpt->GetTimeout() * 1000);
    int nAmount;
    while (true)
    {
      auto t1 = TickCountClock::now();
      if (static_cast<int>((t1-t0).count()) >= nTimeout)
        key = 0;
      else
      {
        nAmount = nTimeout - static_cast<int>((t1-t0).count());
        if (nAmount > 0)
          key = CKey::kbHit(nAmount);
        else
          key = 0;
      }
      if (key == 0)
        key = pOpt->GetDefaultKey();
      else if (key == 1)
      {
        nErrLevel = 255;
        break;
      }
      if ((nErrLevel=pOpt->KeyInOpt(key)) >= 0)
        break;
      else
        std::cerr << '\a' << std::flush; // sound bell -- doesn't work in *nix :(
    }
  }

  if (nErrLevel < 0)
    nErrLevel = 255;
  if (nErrLevel < 255)
  {
    nErrLevel++;
    // uppon success, delete current line
    pOpt->Cleanup();
  }
  delete pOpt;

  return nErrLevel;
}
