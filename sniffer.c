#include <linux/ip.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/skbuff.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/string.h>
#include <linux/ipv6.h>
#include <linux/inet.h>
#include <net/ip.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Evgeny Karssky");
MODULE_DESCRIPTION("Kernel Packet Sniffer");

//имя интерфейса, который слушаем
char ifdev[] = "enp0s3";
int drop = 0;
static struct nf_hook_ops netfilter_ops_out;

int count_not_alow_ip = 0;
char not_allow_ip[][15] = {"127.000.000.001"};

int count_not_alow_ports = 1;
int not_alow_ports[] = {53};

/*
    Подсчёт icmp пакетов со всех интерфейсов. вар 3.
 */ 
static int count_interf = 0;		//колличество интерфесов запомненных
static char interfname[20][20];	//имена интерфесой
static int count_pack[20] = {0};

static int iter = 0;

static void compute(char * interf){
	if(!interf || !(*interf))
		return;
	int i;
	int found = 0;
	for(i = 0; i < count_interf; i++){
		if(strcmp(interfname[i], interf)){
			found = 1;
			break;
		}
	}
	count_pack[i]++;
	if(!found){
		strcpy(interf,interfname[i]);
		count_interf++;
	}
}

static void print_result(void){
	int i;
	for(i = 0; i < count_interf; i++){
		printk("--> interface %s count %d \n", interfname[i], count_pack[i]);
	}
}

static void doCompute(struct sk_buff *skb, const struct nf_hook_state *state){
	struct iphdr* ip_header = (struct iphdr*) skb_network_header(skb);
	if(ip_header->protocol == 1){
		if (state->in) {
			//printk("<>compute in %s \n",state->in->name);
			compute(state->in->name);
			iter++;
		} else if (!state->in && state->out) {
			//printk("<>compute out %s \n",state->out->name);
			compute(state->out->name);
			iter++;
		}
	}
	if(!(iter%5)){
		print_result();
	}
}

//фильтрация по ip адресам
static int ipfilter(char * ip){
	int i;
	for(i = 0; i < count_not_alow_ip; i++){
		if(!strcmp(not_allow_ip[i], ip)){
			return 1;
		}
	}
	return 0;
}

static void intToChar(char *arr, int num){
	*arr = '0'+(num/100);
	*(arr+1) = '0'+(num/10)%10;
	*(arr+2) = '0'+num%10;
}

static int portfilter(__u16 port){
	int i;
	for(i = 0; i < not_alow_ports; i++){
		if(htons(not_alow_ports[i]) == port){
			return 1;
		}
	}
	return 0;
}

//конвертирование кода протокола в строковое имя
inline char * protocol_from_number(int n) {
	switch (n) {
	case 0:
		return "tcp_dummy";
	case 1:
		return "icmp";
	case 2:
		return "igmp";
	case 4:
		return "ipip";
	case 6:
		return "tcp";
	case 8:
		return "egp";
	case 12:
		return "pup";
	case 17:
		return "udp";
	case 22:
		return "idp";
	case 29:
		return "tp";
	case 41:
		return "ipv6_header";
	case 43:
		return "ipv6_routing";
	case 44:
		return "ipv6_fragment";
	case 46:
		return "vsvp";
	case 47:
		return "gre";
	case 50:
		return "esp";
	case 51:
		return "ah";
	case 58:
		return "icmpv6";
	case 59:
		return "ipv6_none";
	case 60:
		return "ipv6_dstopts";
	case 92:
		return "mtp";
	case 98:
		return "encap";
	case 103:
		return "pim";
	case 108:
		return "comp";
	case 132:
		return "sctp";
	case 255:
		return "raw";
	}
	return "unknown";
}

//вывод заголовков пакетов из структуры пакета
void print_packet(struct sk_buff *skb) {
	int dadd, sadd, bit1, bit2, bit3, bit4;
	//IP
	struct iphdr* ip_header = (struct iphdr*) skb_network_header(skb);
	printk("len=%d ", skb->len);
	printk("data_len=%d ", skb->data_len);
	printk("truesize=%d ", skb->truesize);
	printk("frt_id=%d ", ((ip_header->id)));
	printk("frt_offset=%d ", ((ip_header->frag_off)));
	printk("fr_xf=%d ", (ntohs(ip_header->frag_off) & IP_CE) > 0);
	printk("frt_df=%d ", (ntohs(ip_header->frag_off) & IP_DF) > 0);
	printk("fr_mf=%d ", (ntohs(ip_header->frag_off) & IP_MF) > 0);
	printk("\n");
	int port = 0;
	if (ip_header->protocol == 6) {
		//TCP
		struct tcphdr *tc = (struct tcphdr*) skb_transport_header(skb);
		printk("tcp flags: ack= %d ack_seq= %d doff=%d fin=%d seq=%d syn=%d\n",
				tc->ack, tc->ack_seq, tc->doff, tc->fin, tc->seq, tc->syn);
		port = tc->source;
	} else if(ip_header->protocol == 17){
		struct udphdr *tc = (struct udphdr*) skb_transport_header(skb);
		port = tc->source;
	}

	//IP address in xxx.xxx.xxx.xxx -> ....
	dadd = ip_header->daddr;
	sadd = ip_header->saddr;
	bit1 = 255 & sadd;
	bit2 = (0xff00 & sadd) >> 8;
	bit3 = (0xff0000 & sadd) >> 16;
	bit4 = (0xff000000 & sadd) >> 24;
	printk(" %d.%d.%d.%d:%d -> ", bit1, bit2, bit3, bit4, htons(port));
	
	char ip [20] = {0};
    intToChar(ip, bit1);
	ip[3] = '.';
	intToChar(ip+4, bit2);
	ip[7] = '.';
	intToChar(ip+8, bit3);
	ip[11] = '.';
	intToChar(ip+12, bit4);
	
	bit1 = 255 & dadd;
	bit2 = (0xff00 & dadd) >> 8;
	bit3 = (0xff0000 & dadd) >> 16;
	bit4 = (0xff000000 & dadd) >> 24;
	//tcp_header = tcp_hdr(skb);
	printk("---> %d.%d.%d.%d proto %s\n", bit1, bit2, bit3, bit4,
			protocol_from_number(ip_header->protocol));
}

int check_filter(struct sk_buff *skb){
	struct iphdr* ip_header = (struct iphdr*) skb_network_header(skb);
	__u16 ports = 0;
	__u16 portd = 0;
	if (ip_header->protocol == 6) {
		//TCP
		struct tcphdr *tc = (struct tcphdr*) skb_transport_header(skb);
		ports = tc->source;
		portd = tc->dest;
	} else if(ip_header->protocol == 17){
		struct udphdr *tc = (struct udphdr*) skb_transport_header(skb);
		ports = tc->source;
		portd = tc->dest;
	}
	
	int dadd, sadd, bit1, bit2, bit3, bit4;
	
	//IP address in xxx.xxx.xxx.xxx -> ....
	dadd = ip_header->daddr;
	sadd = ip_header->saddr;
	bit1 = 255 & sadd;
	bit2 = (0xff00 & sadd) >> 8;
	bit3 = (0xff0000 & sadd) >> 16;
	bit4 = (0xff000000 & sadd) >> 24;
	
	char sip [20] = {0};
	intToChar(sip, bit1);
	sip[3] = '.';
	intToChar(sip+4, bit2);
	sip[7] = '.';
	intToChar(sip+8, bit3);
	sip[11] = '.';
	intToChar(sip+12, bit4);
		
	bit1 = 255 & dadd;
	bit2 = (0xff00 & dadd) >> 8;
	bit3 = (0xff0000 & dadd) >> 16;
	bit4 = (0xff000000 & dadd) >> 24;
	
	char dip [20] = {0};
	intToChar(dip, bit1);
	dip[3] = '.';
	intToChar(dip+4, bit2);
	dip[7] = '.';
	intToChar(dip+8, bit3);
	dip[11] = '.';
	intToChar(dip+12, bit4);

	return portfilter(ports)+portfilter(portd)+
		   ipfilter(sip)+ipfilter(dip);
}



//конвертирование буфера в структуру skb, для дальнейшей работы
inline int get_raw_data(struct sk_buff *skb, char * buf) {
	int offset;
	char *uk;
	uk = skb->head;
	if (!uk)
		return 0;
	offset = skb_mac_header(skb) - skb->head;
	uk += offset;
	uk += skb->mac_len;//skip mac header
	memcpy(buf, uk, skb->len);
	return skb->len;
}

//главная функция перехвата сообщений
unsigned int main_hook(void *priv, struct sk_buff *skb,
		const struct nf_hook_state *state) {

	struct iphdr *ip_header;
	struct sk_buff* sock_buff;

	//пример получения MTU,
	int mtu = 1500; //MTU
	if (state) {
		if (state->in) {
			mtu = state->in->mtu; //IN interface
			printk("-->interface in %s \n",state->in->name);
		} else if (state->out) {
			mtu = state->out->mtu; // OUT interface
			printk("-->interface out %s \n",state->out->name);
		}
	}

	if (!skb) {
		return NF_ACCEPT;
	}

	//совместимость со старым ядром
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
	sock_buff = skb;
#else
	sock_buff = *skb;
#endif

	if (!sock_buff) {
		return NF_ACCEPT;
	}

	//вытаскиваем ip заголовок
	ip_header = (struct iphdr*) skb_network_header(sock_buff);
	if (!ip_header) {
		printk("---> no ip header!\n");
		return NF_ACCEPT;
	}
	
	print_packet(sock_buff);

	if(check_filter(sock_buff)){
		printk("catched not alow message");
		return NF_DROP;
	}
	
	doCompute(sock_buff, state);
	
	return NF_ACCEPT;
}

int init_module(void) {
	printk("installing sniffer\n");
	netfilter_ops_out.hook = main_hook;
	netfilter_ops_out.pf = PF_INET; //tcp/ipv4
	netfilter_ops_out.hooknum = NF_INET_LOCAL_IN;
	netfilter_ops_out.priority = NF_IP_PRI_FIRST;
	nf_register_net_hook(&init_net, &netfilter_ops_out);
	return 0;
}

static void __exit myexit(void) {
	printk("unregistering sniffer");
	nf_unregister_net_hook(&init_net, &netfilter_ops_out);
	printk("unregistered sniffer");
}

module_exit(myexit);

 
