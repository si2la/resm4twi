/* H 3   T W I   R E S O U R C E   M A N A G E R S   */

/* This is a Resource Manager TWI (I2C) for QNX Neutrino (KPDA)
 *                  Orange Pi One Board
 */


#include <errno.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/resmgr.h>

#include "resm_iface.h"

void options (int argc, char **argv);

/* A resource manager mainly consists of callbacks for POSIX        Менеджер ресурсов в основном состоит из обратных вызовов для POSIX
 * functions a client could call. In the example, we have           функций которые может вызвать клиент. В примере мы имеем
 * callbacks for the open(), read() and write() calls. More are     колбеки для вызовов open(), read() и write(). Возможно и больше.
 * possible. If we don't supply own functions (e.g. for stat(),     Если мы не предоставляем собственные функции (например, для stat(),
 * seek(), etc.), the resource manager framework will use default   seek() и т.д.), платформа resource manager будет использовать сист.
 * system functions, which in most cases return with an error       функции по умолчанию, которые в большинстве случаев возвращают код ошибки,
 * code to indicate that this resource manager doesn't support      указывающий на то, что данный диспетчер ресурсов не поддерживает
 * this function.                                                   эту функцию.*/

/* These prototypes are needed since we are using their names
 * in main() for open(), read() and write() calls*/

int io_open (resmgr_context_t *ctp, io_open_t  *msg, RESMGR_HANDLE_T *handle, void *extra);
int io_read (resmgr_context_t *ctp, io_read_t  *msg, RESMGR_OCB_T *ocb);
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);

/*
 * Our connect and I/O functions - we supply two tables             Наши функции подключения и ввода-вывода - предоставляют две таблицы,
 * which will be filled with pointers to callback functions         которые будут заполнены указателями на функции обратного вызова
 * for each POSIX function. The connect functions are all           для каждой функции POSIX. Функции подключения - это все функции,
 * functions that take a path, e.g. open(), while the I/O           которые задают путь, например open(), в то время как функции ввода
 * functions are those functions that are used with a file          -вывода - это те функции, которые используются с файловым дескриптором
 * descriptor (fd), e.g. read().                                    (fd), например read().
 */

resmgr_connect_funcs_t  connect_funcs;
resmgr_io_funcs_t       io_funcs;

/*
 * Our dispatch, resource manager, and iofunc variables             Наш диспетчер, RM и функции ввода-вывода объявлены здесь.
 * are declared here. These are some small administrative things    Это несколько небольших административных вещей для нашего
 * for our resource manager.                                        менеджера ресурсов.
 */

dispatch_t              *dpp;
resmgr_attr_t           rattr;
dispatch_context_t      *ctp;
iofunc_attr_t           ioattr;

char    *progname = "TWI";
extern int optv;                               // if -v opt used - verbose all operation


// i2c (twi) part
i2c_master_funcs_t  masterf;
i2c_status_t        status;
// handle returned masterf
void *hdl;

int main (int argc, char **argv)
{
    int pathID;

    printf ("Starting...\n");

    /* i2c init part */

    i2c_libversion_t    version;

    i2c_master_getfuncs(&masterf, sizeof(masterf));
    masterf.version_info(&version);
    printf("%s resmanager, version %d.%d.%d\n", progname, version.major, version.minor, version.revision);

    hdl = masterf.init(argc, argv);

    if (hdl == NULL)
    {
        printf ("%s init func fail...\n", progname);

        status = I2C_STATUS_ERROR;

        return status;
    }


    /* end of i2c init part */

    /* Check for command line options (-v only) */
    //options (argc, argv);     // old place of options handler

    /* Allocate and initialize a dispatch structure for use         Выделите и инициализируйте структуру диспетчеризации для исп.
     * by our main loop. This is for the resource manager           нашим основным циклом.  Это нужно для использования фреймворка
     * framework to use. It will receive messages for us,           resource manager.  Он будет получать сообщения для нас,
     * analyze the message type integer and call the matching       анализировать тип сообщения и вызывать колбек обработчик
     * handler callback function (i.e. io_open, io_read, etc.)      (io_open, io_read, и т.д.)*/
    dpp = dispatch_create ();
    if (dpp == NULL) {
        fprintf (stderr, "%s:  couldn't dispatch_create: %s\n",
                 progname, strerror (errno));
        exit (1);
    }

    /* Set up the resource manager attributes structure. We'll      Настройте структуру атрибутов менеджера ресурсов.
     * use this as a way of passing information to                  Мы будем использовать это как способ передачи информации в
     * resmgr_attach(). The attributes are used to specify          resmgr_attach().  Атрибуты используются для указания
     * the maximum message length to be received at once,           максимальной длины сообщения, которое должно быть получено за один раз,
     * and the number of message fragments (iov's) that             и количества фрагментов сообщения (iov),
     * are possible for the reply.                                  которые могут быть использованы для ответа.
     * For now, we'll just use defaults by setting the              На данный момент мы просто будем использовать значения по умолчанию,
     * attribute structure to zeroes.                               установив в структуре атрибутов значения, равные нулю.*/
    memset (&rattr, 0, sizeof (rattr));

    /* Now, let's intialize the tables of connect functions and     Теперь давайте инициализируем таблицы функций подключения
     * I/O functions to their defaults (system fallback             и функций ввода-вывода к их значениям по умолчанию (системные резервные
     * routines) and then override the defaults with the            процедуры), а затем переопределим значения по умолчанию с помощью
     * functions that we are providing.                             функций, которые мы предоставляем.*/
    iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                      _RESMGR_IO_NFUNCS, &io_funcs);
    /* Now we override the default function pointers with
     * some of our own coded functions: */
    connect_funcs.open = io_open;
    io_funcs.read = io_read;
    io_funcs.write = io_write;

    /* Initialize the device attributes for the particular          Инициализируем атрибуты устройства для конкретного
     * device name we are going to register. It consists of         имени устройства, которое мы собираемся зарегистрировать.
     * permissions, type of device, owner and group ID */
    iofunc_attr_init (&ioattr, S_IFCHR | 0666, NULL, NULL);

    /* Next we call resmgr_attach() to register our device name     Затем мы вызываем resmgr_attach(), чтобы зарегистрировать имя нашего
     * with the process manager, and also to let it know about      устройства в диспетчере процессов, а также сообщить ему о
     * our connect and I/O functions.                               наших функциях подключения и ввода-вывода.*/
    pathID = resmgr_attach (dpp, &rattr, "/dev/twi0",
                            _FTYPE_ANY, 0, &connect_funcs, &io_funcs, &ioattr);
    if (pathID == -1) {
        fprintf (stderr, "%s:  couldn't attach pathname: %s\n",
                 progname, strerror (errno));
        exit (1);
    }

    /* Now we allocate some memory for the dispatch context         Теперь мы выделяем немного памяти для структуры контекста диспетчера (ctp),
     * structure, which will later be used when we receive          которая позже будет использоваться при получении сообщений.
     * messages. */
    ctp = dispatch_context_alloc (dpp);

    /* Done! We can now go into our "receive loop" and wait         Теперь мы можем перейти к нашему "циклу приема" и ждать сообщений.
     * for messages. The dispatch_block() function is calling       Функция dispatch_block() вызывает функцию MsgReceive() незаметно
     * MsgReceive() under the covers, and receives for us.          и принимает сообщения для нас.
     * The dispatch_handler() function analyzes the message         Функция dispatch_handler() анализирует сообщение
     * for us and calls the appropriate callback function.          и вызывает соответствующую функцию обратного вызова.*/
    while (1) {
        if ((ctp = dispatch_block (ctp)) == NULL) {
            fprintf (stderr, "%s:  dispatch_block failed: %s\n",
                     progname, strerror (errno));
            exit (1);
        }
        /* Call the correct callback function for the message       Вызовите правильную функцию обратного вызова для
         * received. This is a single-threaded resource manager,    полученного сообщения. Это однопоточный менеджер
         * so the next request will be handled only when this       ресурсов, поэтому следующий запрос будет обработан
         * call returns. Consult our documentation if you want      только при повторном вызове. Если вы хотите создать
         * to create a multi-threaded resource manager.             многопоточный менеджер ресурсов, обратитесь к нашей документации.*/
        dispatch_handler (ctp);
    }
}

/*
 *  io_open
 *
 * We are called here when the client does an open().               Здесь мы вызываемся, когда клиент выполняет open()
 * In this simple example, we just call the default routine         В этом простом примере мы просто вызываем процедуру по умолчанию
 * (which would be called anyway if we did not supply our own       (которая была бы вызвана в любом случае, если бы мы не предоставили
 * callback), which creates an OCB (Open Context Block) for us.     свой собственный обратный вызов), кот. создает для нас OCB (Open Context Block).
 * In more complex resource managers, you will want to check if     Например, в более сложных менеджерах ресурсов вам нужно будет проверить,
 * the hardware is available, for example.                          доступно ли оборудование.
 */

int
io_open (resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle, void *extra)
{
    if (optv) {
        printf ("%s: in io_open..  ", progname);
    }

    return (iofunc_open_default (ctp, msg, handle, extra));
}

/*
 *  io_read
 *
 *  At this point, the client has called the library read()         На этом этапе клиент вызвал библиотечную функцию read()
 *  function, and expects zero or more bytes.  Since this is        и ожидает ноль или более байт.  Поскольку это диспетчер
 *  the /dev/Null resource manager, we return zero bytes to         ресурсов /dev/Null, мы возвращаем ноль байт, чтобы
 *  indicate EOF -- no more bytes expected.                         указать "EOF - больше байт не ожидается".
 */

/* The message that we received can be accessed via the             К полученному нами сообщению можно получить доступ с помощью
 * pointer *msg. A pointer to the OCB that belongs to this          указателя *msg. Указатель на OCB, который относится к этому read() - *ocb.
 * read is the *ocb. The *ctp pointer points to a context           Указатель *ctp указывает на труктуру контекста, кот. используется
 * structure that is used by the resource manager framework         фреймворком resource manager для определения того, кому отвечать,
 * to determine whom to reply to, and more.                         и многое другое.*/
int
io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
    int status;

    if (optv) {
        printf ("%s: in io_read..  ", progname);
    }

    /* Here we verify if the client has the access                  Здесь мы проверяем, есть ли у клиента права доступа,
     * rights needed to read from our device                        необходимые для чтения с нашего устройства*/
    if ((status = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* We check if our read callback was called because of          Мы проверяем, был ли вызван обратный вызов read из pread()
     * a pread() or a normal read() call. If pread(), we return     или из обычного read(). Если pread(), то возвращаем
     * with an error code indicating that we don't support it.      ошибку - мы это не поддерживаем*/
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return (ENOSYS);
    }
    /* Here we set the number of bytes we will return. As we        Здесь мы задаем количество байт, которые будем возвращать.
     * are the null device, we will return 0 bytes. Normally,       Поскольку мы являемся нулевым устройством, мы вернем 0 байт.
     * here you would write the number of bytes you                 Обычно здесь указывается количество байт, которое вы фактически
     * are actually returning.                                      возвращаете.*/
    _IO_SET_READ_NBYTES(ctp, 0);

    /* The next line (commented) is used to tell the system how     Следующая строка (с комментариями) используется, чтобы сообщить системе,
     * large your buffer is in which you want to return your        насколько велик ваш буфер, в который вы хотите вернуть свои данные
     * data for the read() call. We don't set it here because       для вызова read(). Мы не указываем его здесь, потому что
     * as a null device, we return 0 data. See the "time            нулевое устройство возвращает 0 данных. Смотреть пример
     * resource manager" example for an actual use.                 TODO "time resource manager" !!!
     */
    //	SETIOV( ctp->iov, buf, sizeof(buf));

    if (msg->i.nbytes > 0) {
        ocb->attr->flags |= IOFUNC_ATTR_ATIME;
    }

    /* We return 0 parts, because we are the null device.           Мы возвращаем 0 частей, потому что мы являемся нулевым устройством.
     * Normally, if you return actual data, you would return at     Обычно, если вы возвращаете фактические данные, вы возвращаете
     * least 1 part. A pointer to and a buffer length for 1 part    как минимум 1 часть. Указатель и длина буфера для 1 части
     * are located in the ctp structure.                            находятся в структуре ctp.*/
    return (_RESMGR_NPARTS (0));
    /* What you return could also be several chunks of
     * data. In this case, you would return more
     * than 1 part. See the "time resource manager" example
     * on how to use this. */

}

/*
 *  io_write
 *
 *  At this point, the client has called the library write()        На этом этапе клиент вызвал функцию write()
 *  function, and expects that our resource manager will write      и ожидает, что наш менеджер ресурсов запишет в устройство
 *  the number of bytes that have been specified to the device.     указанное количество байт.
 *
 *  Since this is /dev/Null, all of the clients' writes always      Поскольку это /dev/Null, все записи клиентов всегда работают
 *  work -- they just go into Deep Outer Space.                     - уходят в в dev-null
 */

int
io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb)
{
    int status;
    char *buf;

    if (optv) {
        printf ("%s: in io_write..  ", progname);
    }

    /* Check the access permissions of the client */
    if ((status = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK) {
        return (status);
    }

    /* Check if pwrite() or normal write() */
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
        return (ENOSYS);
    }

    /* Set the number of bytes successfully written for             Задайте количество байт, успешно записанных для клиента.
     * the client. This information will be passed to the           Эта информация будет передана клиенту resource manager'ом
     * client by the resource manager framework upon reply.         после получения ответа.
     * In this example, we just take the number of  bytes that      В этом примере мы просто берем количество байт, которые были отправлены нам,
     * were sent to us and claim we successfully wrote them.        и заявляем, что мы их успешно записали.*/
    _IO_SET_WRITE_NBYTES (ctp, msg -> i.nbytes);
    printf("got write of %d bytes, data:\t", msg->i.nbytes);

    /* Here we print the data. This is a good example for the case  Здесь мы просто выводим данные. Для примера. Можно еще делать что-то..
     * where you actually would like to do something with the data.
     */

    /* First check if our message buffer was large enough           Сначала проверка, достаточно ли велик наш буфер сообщений,
     * to receive the whole write at once. If yes, print data.      если да, печать */
    if( ((_Uint32t)msg->i.nbytes <= ctp->info.msglen - ctp->offset - sizeof(msg->i)) &&
            ((_Uint32t)ctp->info.msglen < ctp->msg_max_size))  { // space for NUL byte
        buf = (char *)(msg+1);
        //buf[msg->i.nbytes] = '0';
        buf[msg->i.nbytes] = '\n';
        printf("%s", buf );
    } else {
        /* If we did not receive the whole message because the          Если мы не получили все сообщение целиком, потому что
        * client wanted to send more than we could receive, we          клиент хочет послать больше, чем мы можем принять,
        * allocate memory for all the data and use resmgr_msgread()     мы выделяем память для всех данных и используем resmgr_msgread()
        * to read all the data at once. Although we did not receive     для чтения всех данных сразу. Хотя сначала мы не получили
        * the data completely first, because our buffer was not big     данные полностью, потому что наш буфер был недостаточно большим,
        * enough, the data is still fully available on the client       данные по-прежнему полностью доступны на стороне клиента,
        * side, because its write() call blocks until we return         потому что это блокирует вызов write() до тех пор,
        * from this callback!                                           пока мы не вернемся из этого обратного вызова! */
        buf = malloc( msg->i.nbytes + 1);
        resmgr_msgread( ctp, buf, msg->i.nbytes, sizeof(msg->i));
        buf[msg->i.nbytes] = '0';
        printf("%s", buf );
        free(buf);
    }

    /* work TWI here */
    uint8_t * twi_msgbuf;
    uint * twi_info;
    twi_msgbuf = malloc(sizeof(uint8_t)*3);
    uint twi_nbytes;

    //masterf.ctl(hdl, 0xE3, twi_msgbuf, 3, &twi_nbytes, twi_info);
    masterf.ctl(hdl, atoh(buf), twi_msgbuf, 3, &twi_nbytes, twi_info);

    /* Finally, if we received more than 0 bytes, we mark the       Наконец, если мы получили более 0 байт, мы помечаем
    * file information for the device to be updated:                какая информация в файле устройства будет изменена:
    * modification time and change of file status time. To          время модификации и время изменения статусной информации файла.
    * avoid constant update of the real file status information     Чтобы избежать постоянного обновления статусной информации
    * (which would involve overhead getting the current time), we   (что повлекло бы за собой накладные расходы на получение текущего времени)
    * just set these flags. The actual update is done upon          мы просто устанавливаем эти флаги. Фактическое обновление выполняется
    * closing, which is valid according to POSIX.                   после закрытия, что является допустимым в соответствии с POSIX. */
    if (msg->i.nbytes > 0) {
        ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;
    }

    return (_RESMGR_NPARTS (0));
}

/* Why we don't have any close callback? Because the default        Почему у нас нет обратного вызова close? Потому что ф. по-умолч.
 * function, iofunc_close_ocb_default(), does all we need in this   iofunc_close_ocb_default(), выполняет все, что нам нужно в этом случае:
 * case: Free the ocb, update the time stamps etc. See the docs     освобождает ocp, обновляет временные метки и т.д.
 * for more info.                                                   Доп. информацию смотрите в документации.
 */


/*
 *  options
 *
 *  This routine handles the command-line options.
 *  For our simple /dev/Null, we support:
 *      -v      verbose operation
 */

void
options (int argc, char **argv)
{
    int     opt;
    optv = 0;
    while (optind < argc) {
        while ((opt = getopt (argc, argv, "v")) != -1) {
            switch (opt) {
            case 'v':
                optv = 1;
                printf("optv = %d", optv);
                break;
            }
        }
    }
}
