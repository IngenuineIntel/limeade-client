// main.c

#include<linux/cred.h>
#include<linux/hashtable.h>
#include<linux/kprobes.h>
#include<linux/module.h>

#include "lce.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roan Rothrock");
MODULE_DESCRIPTION("Limeade Client Engine");
MODULE_VERSION("0.1");

static const struct proc_ops lce_proc_ops = {
  .proc_open = lce_proc_open,
  .proc_read = lce_proc_read,
  .proc_lseek = noop_llseek,
};

static int lce_register_kprobes(void)
{
  int ret;
  for(int i = 0; i < lce_nr_kprobes; i++)
  {
    ret = register_kprobe(&lce_kprobes[i]);
    if(ret < 0)
    {
      pr_err("lce: failed to register kprobe for symbol %s (returned %i)\n",
             lce_kprobes[i].symbol_name, ret);

      /*
      while(--i >= 0)
        unregister_kprobe(&lce_kprobes[i]);
      return ret;
      */
    }
  }
  return 0;
}

static int lce_register_kretprobes(void)
{
  int ret;
  for(int i = 0; i < lce_nr_kretprobes; i++)
  {
    ret = register_kretprobe(&lce_kretprobes[i]);
    if(ret < 0)
    {
      pr_err("lce: failed to resgister kretprobe for symbol %s (return %i)\n",
             lce_kretprobes[i].kp.symbol_name, ret);

      /*
      while(--i >= 0)
        unregister_kretprobe(&lce_kretprobes[i]);
      return ret;
      */
    }
  }
  return 0;
}

static int __init lce_init(void)
{
  int ret, i;

  INIT_KFIFO(lce_kfifo);
  hash_init(lce_hashtbl);
  
  lce_proc_entry = proc_create(LCE_PROCFILE_PATH, LCE_PROCFILE_PERM,
                               NULL, &lce_proc_ops);
  if(!lce_proc_entry)
  {
    pr_err("lce: failed to create procfile \"%s\"\n", LCE_PROCFILE_PATH);
    proc_remove(lce_proc_entry);
    return -1;
  }

  ret = lce_register_kprobes();
  if(ret)
    goto err_1;

  ret = lce_register_kretprobes();
  if(ret)
    goto err_2;

  atomic_set(&lce_ready, 1);

  pr_info("lce: loaded (%i kprobes active, %i kretprobes active, procfile @ \"%s\"\n",
          lce_nr_kprobes, lce_nr_kretprobes, LCE_PROCFILE_PATH);

  return 0;

err_2:
  for(i = 0; i < lce_nr_kretprobes; i++)
    unregister_kprobe(&lce_kprobes[i]);
err_1:
  proc_remove(lce_proc_entry);
  return ret;
}

static void __exit lce_exit(void)
{
  struct lce_event_pending *entry;
  struct hlist_node *tmp_node;
  unsigned long lock_flags;
  int bucket, i;

  atomic_set(&lce_ready, 0);

  spin_lock_irqsave(&lce_hashtbl_lock, lock_flags);
  hash_for_each_safe(lce_hashtbl, bucket, tmp_node, entry, node)
  {
    hash_del(&entry->node);
    kfree(entry);
  }
  spin_unlock_irqrestore(&lce_hashtbl_lock, lock_flags);

  for(i = 0; i < lce_nr_kretprobes; i++)
  {
    if(!lce_kretprobes[i].kp.addr)
      continue;
    unregister_kretprobe(&lce_kretprobes[i]);
    pr_info("unregistered kretprobe @ symbol %s", lce_kretprobes[i].kp.symbol_name);
  }

  for(i = 0; i < lce_nr_kprobes; i++)
  {
    if(!lce_kprobes[i].addr)
      continue;
    unregister_kprobe(&lce_kprobes[i]);
    pr_info("unregistered kprobe @ symbol %s", lce_kprobes[i].symbol_name);
  }

  proc_remove(lce_proc_entry);

  pr_info("lce: ejected\n");
}

module_init(lce_init);
module_exit(lce_exit);

